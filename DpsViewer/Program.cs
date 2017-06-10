﻿using System;
using FFXIVAPP.Memory;
using FFXIVAPP.Memory.Models;
using System.Diagnostics;
using FFXIVAPP.Memory.Core;
using FFXIVAPP.Memory.Helpers;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO.Pipes;
using System.IO;
using System.Linq;
using System.Text;

namespace DpsViewer
{
	partial class Program
	{
		protected delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		protected static extern int GetWindowText(IntPtr hWnd, StringBuilder strText, int maxCount);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		protected static extern int GetWindowTextLength(IntPtr hWnd);
		[DllImport("user32.dll")]
		protected static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
		[DllImport("user32.dll")]
		protected static extern bool IsWindowVisible(IntPtr hWnd);
		[DllImport("user32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool SetForegroundWindow(IntPtr hWnd);

		[DllImport("user32.dll", CharSet = CharSet.Auto)]
		static extern IntPtr SendMessage(IntPtr hWnd, UInt32 Msg, int wParam, int lParam);

		[DllImport("user32.dll", SetLastError = true)]
		static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

		[DllImport("kernel32.dll")]
		static extern void ExitProcess(uint uExitCode);

		const uint WM_KEYDOWN = 0x100;
		const uint WM_KEYUP = 0x101;

		public static string version;

		static IntPtr ffxivhWnd;
		static Process process;

		static IntPtr injectedProcess;
		static IntPtr injectedDll;

		public static void quit()
		{
			try {
				pipeChatReader.Close();
				writer.Close();
			} catch (Exception) { }
			ejectDll(injectedProcess, injectedDll);
			ExitProcess(0);
		}

		static NamedPipeClientStream pipeChatWriter;
		static NamedPipeClientStream pipeChatReader;
		static StreamWriter writer;

		static void readAsync()
		{
			byte[] readBuf = new byte[1024];
			pipeChatReader.ReadAsync(readBuf, 0, readBuf.Length).ContinueWith(t => {
				if (!t.IsCompleted)
					return;
				readAsync();
			});
		}

		protected static bool EnumTheWindows(IntPtr hWnd, IntPtr lParam)
		{
			int size = GetWindowTextLength(hWnd);
			if (size++ > 0 && IsWindowVisible(hWnd)) {
				StringBuilder sb = new StringBuilder(size);
				GetWindowText(hWnd, sb, size);
				if (sb.ToString().StartsWith("FINAL FANTASY XIV")) {
					uint pid;
					GetWindowThreadProcessId(hWnd, out pid);
					if (pid == process.Id) {
						ffxivhWnd = hWnd;
						return false;
					}
				}
			}
			return true;
		}

		[STAThread]
		static void Main(string[] args)
		{
			Application.EnableVisualStyles();
			bool dx11 = false;
			List<Process> processes = new List<Process>(Process.GetProcessesByName("ffxiv"));
			processes.RemoveAll((item) => {
				try {
					return item.HasExited;
				} catch (Exception) {
					return true;
				}
			});
			if (processes.Count == 0) {
				dx11 = true;
				processes = new List<Process>(Process.GetProcessesByName("ffxiv_dx11"));
				processes.RemoveAll((item) => {
					try {
						return item.HasExited;
					} catch (Exception) {
						return true;
					}
				});
			}
			if (processes.Count > 0) {
				string gameLanguage = "English";

				if (processes.Count > 1 && args.Length == 0) {
					string k = System.Reflection.Assembly.GetExecutingAssembly().CodeBase;
					foreach (var p in processes)
						Process.Start(k, p.Id.ToString());
					return;
				}
				process = processes[0];
				if (processes.Count > 1 && args.Length > 0)
					foreach (var p in processes)
						if (p.Id.ToString() == args[0])
							process = p;
				if (process == null) {
					MessageBox.Show("FFXIV process not found");
					return;
				}
				EnumWindows(EnumTheWindows, IntPtr.Zero);
				if (ffxivhWnd.ToInt32() == 0) {
					MessageBox.Show("FFXIV window not found");
					return;
				}
				ProcessModel processModel = new ProcessModel {
					Process = process,
					IsWin64 = dx11
				};
				version = "latest";
				if (process.MainModule.FileVersionInfo.FileName.Contains("KOREA"))
					version = "KOR";
				MemoryHandler.Instance.SetProcess(processModel, gameLanguage, version);
				// MemoryHandler.Instance.Structures.ActorEntity.GatheringInvisible = 248;

				while (!Scanner.Instance.Locations.ContainsKey("CHARMAP") ||
					!Scanner.Instance.Locations.ContainsKey("TARGET"))
					Thread.Sleep(100);

				// Scanner.Instance.Locations["CHARMAP"].SigScanAddress = new IntPtr(Scanner.Instance.Locations["CHARMAP"].SigScanAddress.ToInt64() + process.MainModule.BaseAddress.ToInt64());
				foreach(var a in Reader.GetActors().MonsterEntities) {
					Debug.Print(a.Value.Name + ": " + a.Value.ID.ToString("X8") + " / " + a.Value.Pointer.ToString("X16"));
				}
				Reader.GetPartyMembers();
				Reader.GetTargetInfo();

				string dllFN = "FFXIVDLL_" + (dx11 ? "x64" : "x86") + ".dll";


				StreamWriter info = new StreamWriter(AppDomain.CurrentDo‌​main.BaseDirectory + "FFXIVDLLInfo_" + process.Id + ".txt");
				info.WriteLine(Scanner.Instance.Locations["CHARMAP"].SigScanAddress.ToInt64());
				info.WriteLine(MemoryHandler.Instance.Structures.ActorEntity.ID);
				info.WriteLine(MemoryHandler.Instance.Structures.ActorEntity.Name);
				info.WriteLine(MemoryHandler.Instance.Structures.ActorEntity.OwnerID);
				info.WriteLine(MemoryHandler.Instance.Structures.ActorEntity.Type);
				info.WriteLine(MemoryHandler.Instance.Structures.ActorEntity.Job);
				info.WriteLine(((IntPtr)Scanner.Instance.Locations["TARGET"]).ToInt64());
				info.WriteLine(MemoryHandler.Instance.Structures.TargetInfo.Current);
				info.WriteLine(MemoryHandler.Instance.Structures.TargetInfo.MouseOver);
				info.WriteLine(MemoryHandler.Instance.Structures.TargetInfo.Focus);
				info.Close();
				if (ejectDll(MemoryHandler.Instance.ProcessHandle, AppDomain.CurrentDo‌​main.BaseDirectory + dllFN))
					return;
				injectedDll = InjectDLL(injectedProcess = MemoryHandler.Instance.ProcessHandle,
					AppDomain.CurrentDo‌​main.BaseDirectory + dllFN);

				SetForegroundWindow(ffxivhWnd);
				ExitProcess(0);

				pipeChatWriter = new NamedPipeClientStream(".", "ffxivchatinject_" + process.Id, PipeDirection.InOut);
				pipeChatReader = new NamedPipeClientStream(".", "ffxivchatstream_" + process.Id, PipeDirection.InOut);
				pipeChatWriter.Connect();
				pipeChatReader.Connect();
				pipeChatReader.ReadMode = PipeTransmissionMode.Message;
				writer = new StreamWriter(pipeChatWriter);
				writer.AutoFlush = true;
				readAsync();

				Info f = new Info();
				f.Show();
				Application.Run(f);
			}
			quit();
		}
	}
}
