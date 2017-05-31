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

using System;
using System.Collections.Generic;
using FFXIVAPP.Memory.Core;
using FFXIVAPP.Memory.Core.Enums;
using FFXIVAPP.Memory.Models;

namespace FFXIVAPP.Memory
{
    public static partial class Reader
    {
        public static IntPtr InventoryPointerMap { get; set; }

        public static InventoryReadResult GetInventoryItems()
        {
            var result = new InventoryReadResult();

            if (Scanner.Instance.Locations.ContainsKey("INVENTORY"))
            {
                try
                {
                    InventoryPointerMap = new IntPtr(MemoryHandler.Instance.GetPlatformUInt(Scanner.Instance.Locations["INVENTORY"]));

                    result.InventoryEntities = new List<InventoryEntity>
                    {
                        GetItems(InventoryPointerMap, Entity.Container["INVENTORY_1"]),
                        GetItems(InventoryPointerMap, Entity.Container["INVENTORY_2"]),
                        GetItems(InventoryPointerMap, Entity.Container["INVENTORY_3"]),
                        GetItems(InventoryPointerMap, Entity.Container["INVENTORY_4"]),
                        GetItems(InventoryPointerMap, Entity.Container["CURRENT_EQ"]),
                        GetItems(InventoryPointerMap, Entity.Container["EXTRA_EQ"]),
                        GetItems(InventoryPointerMap, Entity.Container["CRYSTALS"]),
                        GetItems(InventoryPointerMap, Entity.Container["QUESTS_KI"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_1"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_2"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_3"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_4"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_5"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_6"]),
                        GetItems(InventoryPointerMap, Entity.Container["HIRE_7"]),
                        GetItems(InventoryPointerMap, Entity.Container["COMPANY_1"]),
                        GetItems(InventoryPointerMap, Entity.Container["COMPANY_2"]),
                        GetItems(InventoryPointerMap, Entity.Container["COMPANY_3"]),
                        GetItems(InventoryPointerMap, Entity.Container["COMPANY_CRYSTALS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_MH"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_OH"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_HEAD"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_BODY"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_HANDS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_BELT"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_LEGS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_FEET"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_EARRINGS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_NECK"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_WRISTS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_RINGS"]),
                        GetItems(InventoryPointerMap, Entity.Container["AC_SOULS"])
                    };
                }
                catch (Exception)
                {
                    // ignored
                }
            }

            return result;
        }

        private static InventoryEntity GetItems(IntPtr address, byte typeID)
        {
            var offset = (uint) (typeID * 24);
            var containerAddress = MemoryHandler.Instance.GetPlatformUInt(address, offset);
            var type = Entity.Container[typeID];
            var container = new InventoryEntity
            {
                Amount = MemoryHandler.Instance.GetByte(address, offset + MemoryHandler.Instance.Structures.InventoryEntity.Amount),
                Items = new List<ItemInfo>(),
                TypeID = typeID,
                Type = type
            };
            // The number of item is 50 in COMPANY's locker
            int limit;
            switch (type)
            {
                case "COMPANY_1":
                case "COMPANY_2":
                case "COMPANY_3":
                    limit = 3200;
                    break;
                default:
                    limit = 1600;
                    break;
            }

            for (var ci = 0; ci < limit; ci += 64)
            {
                var itemOffset = new IntPtr(containerAddress + ci);
                var id = MemoryHandler.Instance.GetPlatformUInt(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.ID);
                if (id > 0)
                {
                    container.Items.Add(new ItemInfo
                             {
                                 ID = (uint) id,
                                 Slot = MemoryHandler.Instance.GetByte(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.Slot),
                                 Amount = MemoryHandler.Instance.GetByte(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.Amount),
                                 SB = MemoryHandler.Instance.GetUInt16(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.SB),
                                 Durability = MemoryHandler.Instance.GetUInt16(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.ID),
                                 GlamourID = (uint) MemoryHandler.Instance.GetPlatformUInt(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.GlamourID),
                                 //get the flag that show if the item is hq or not
                                 IsHQ = (MemoryHandler.Instance.GetByte(itemOffset, MemoryHandler.Instance.Structures.ItemInfo.IsHQ) == 0x01)
                             });
                }
            }

            return container;
        }
    }
}
