/*
* Liam Ashdown
* Copyright (C) 2019
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _ITEM_ITEM_h
#define _ITEM_ITEM_h
#include "Common/SharedDefines.h"
#endif /* _ITEM_ITEM_h */

namespace SteerStone
{
    class Item
    {
    public:
        friend class ItemManager;

    public:
        /// Constructor
        Item();

        /// Deconstructor
        ~Item();

    public:

        /// CanBeWalkedOn
        /// Check if tile can be walked on
        /// @p_Extra : If true do extra checks
        bool CanBeWalkedOn(bool m_Extra = false);

        /// Item Info
        uint32 GetId()                                const { return m_Id;                }
        uint32 GetSpriteId()                          const { return m_SpriteId;          }
        std::string GetName()                         const { return m_Name;              }
        std::string GetDescription()                  const { return m_Description;       }
        std::string GetColour()                       const { return m_Colour;            }
        std::string GetRoomModel()                    const { return m_RoomModel;         }
        std::string GetSprite()                       const { return m_Sprite;            }
        int16 GetPositionX()                          const { return m_X;                 }
        int16 GetPositionY()                          const { return m_Y;                 }
        double GetPositionZ()                         const { return m_Z;                 }
        int16 GetRotation()                           const { return m_Rotation;          }
        float GetTopHeight()                          const { return m_TopHeight;         }
        int16 GetLength()                             const { return m_Length;            }
        int16 GetWidth()                              const { return m_Width;             }
        std::vector<std::string> GetTrigger()         const { return m_Trigger;           }
        std::string GetProgram()                      const { return m_Program;           }
        std::string GetState()                        const { return m_State;             }
        bool IsTradable()                             const { return m_Tradable;          }
        bool IsRecycleable()                          const { return m_Recycleable;       }
        uint32 GetDrinkId()                           const { return m_DrinkId;           }

        void SetState(std::string const p_State)            { m_State = p_State;          }

    private:
        /// Variables
        uint32 m_Id;
        uint32 m_SpriteId;
        std::string m_Name;
        std::string m_Description;
        std::string m_Colour;
        std::string m_RoomModel;
        std::string m_Sprite;
        int16 m_X;
        int16 m_Y;
        double m_Z;
        int16 m_Rotation;
        float m_TopHeight;
        int16 m_Length;
        int16 m_Width;
        std::vector<std::string> m_Trigger;
        std::string m_Program;
        std::string m_State;                ///< Program State
        bool m_Tradable;
        bool m_Recycleable;
        uint32 m_DrinkId;
    };
} ///< NAMESPACE STEERSTONE