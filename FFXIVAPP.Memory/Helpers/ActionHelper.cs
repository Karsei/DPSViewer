// FFXIVAPP.Memory
// FFXIVAPP & Related Plugins/Modules
// Copyright ?2007 - 2016 Ryan Wilson - All Rights Reserved
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

using System.Collections.Concurrent;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using FFXIVAPP.Memory.Models;
using Newtonsoft.Json;

namespace FFXIVAPP.Memory.Helpers
{
    public static class ActionHelper
    {
        private static ConcurrentDictionary<uint, ActionItem> _actions;

        private static ConcurrentDictionary<uint, ActionItem> Actions
        {
            get { return _actions ?? (_actions = new ConcurrentDictionary<uint, ActionItem>()); }
            set
            {
                if (_actions == null)
                {
                    _actions = new ConcurrentDictionary<uint, ActionItem>();
                }
                _actions = value;
            }
        }

        public static ActionItem ActionInfo(uint id)
        {
            lock (Actions)
            {
                if (!Actions.Any())
                {
                    Generate();
                }
                if (Actions.ContainsKey(id))
                {
                    return Actions[id];
                }
                return new ActionItem
                {
                    Name = new Localization
                    {
                        Chinese = "???",
                        English = "???",
                        French = "???",
                        German = "???",
                        Japanese = "???",
                        Korean = "???"
                    }
                };
            }
        }

        private static void Generate()
        {
            var file = Path.Combine(Directory.GetCurrentDirectory(), "actions.json");
            if (File.Exists(file))
            {
                using (var streamReader = new StreamReader(file))
                {
                    var json = streamReader.ReadToEnd();
                    Actions = JsonConvert.DeserializeObject<ConcurrentDictionary<uint, ActionItem>>(json, Constants.SerializerSettings);
                }
            }
            else
            {
                using (var webClient = new WebClient
                {
                    Encoding = Encoding.UTF8
                })
                {
                    var json = webClient.DownloadString("http://xivapp.com/api/actions");
                    Actions = JsonConvert.DeserializeObject<ConcurrentDictionary<uint, ActionItem>>(json, Constants.SerializerSettings);
                    File.WriteAllText(file, JsonConvert.SerializeObject(Actions, Formatting.Indented, Constants.SerializerSettings), Encoding.UTF8);
                }
            }
        }
    }
}
