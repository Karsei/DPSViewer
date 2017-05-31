﻿// FFXIVAPP.Memory
// FFXIVAPP & Related Plugins/Modules
// Copyright © 2007 - 2016 Ryan Wilson - All Rights Reserved
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
    public static class ZoneHelper
    {
        private static ConcurrentDictionary<uint, MapItem> _mapInfos;

        private static ConcurrentDictionary<uint, MapItem> MapInfos
        {
            get { return _mapInfos ?? (_mapInfos = new ConcurrentDictionary<uint, MapItem>()); }
            set
            {
                if (_mapInfos == null)
                {
                    _mapInfos = new ConcurrentDictionary<uint, MapItem>();
                }
                _mapInfos = value;
            }
        }

        public static MapItem MapInfo(uint id)
        {
            lock (MapInfos)
            {
                if (!MapInfos.Any())
                {
                    Generate();
                }
                if (MapInfos.ContainsKey(id))
                {
                    return MapInfos[id];
                }
                return new MapItem
                {
                    Name = new Localization
                    {
                        Chinese = "???",
                        English = "???",
                        French = "???",
                        German = "???",
                        Japanese = "???",
                        Korean = "???"
                    },
                    Index = 0,
                    IsDungeonInstance = false
                };
            }
        }

        private static void Generate()
        {
            //
            // These ID's link to offset 7 in the old JSON values.
            // eg: "map id = 4" would be 148 in offset 7.
            // This is known as the TerritoryType value
            // - It maps directly to SaintCoins map.csv against TerritoryType ID
            //
            var file = Path.Combine(Directory.GetCurrentDirectory(), "zones.json");
            if (File.Exists(file))
            {
                using (var streamReader = new StreamReader(file))
                {
                    var json = streamReader.ReadToEnd();
                    MapInfos = JsonConvert.DeserializeObject<ConcurrentDictionary<uint, MapItem>>(json, Constants.SerializerSettings);
                }
            }
            else
            {
                using (var webClient = new WebClient
                {
                    Encoding = Encoding.UTF8
                })
                {
                    var json = webClient.DownloadString("http://xivapp.com/api/zones");
                    MapInfos = JsonConvert.DeserializeObject<ConcurrentDictionary<uint, MapItem>>(json, Constants.SerializerSettings);
                    File.WriteAllText(file, JsonConvert.SerializeObject(MapInfos, Formatting.Indented, Constants.SerializerSettings), Encoding.UTF8);
                }
            }
        }
    }
}
