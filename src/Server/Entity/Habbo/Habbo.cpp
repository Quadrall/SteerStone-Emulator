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

#include "Habbo.h"
#include "Hotel.h"
#include "RoomManager.h"
#include "Opcode/Packets/Server/HabboInfoPackets.h"
#include "Opcode/Packets/Server/MessengerPackets.h"
#include "Opcode/Packets/Server/FavouriteRoomPackets.h"
#include "Opcode/Packets/Server/HabboInfoPackets.h"
#include "Opcode/Packets/Server/MiscPackets.h"
#include "Opcode/Packets/Server/RoomPackets.h"

namespace SteerStone
{
    /// Constructor
    /// @p_HabboSocket -Socket is inheriting from
    Habbo::Habbo(HabboSocket* p_HabboSocket)
        : m_Socket(p_HabboSocket ? p_HabboSocket->Shared<HabboSocket>() : nullptr), m_RoomGUID(0)
    {
        m_PingInterval    = sConfig->GetIntDefault("PongInterval", 30000);
        m_UpdateAccount   = sConfig->GetIntDefault("PlayerAccountUpdate", 600000);
        m_MaxFriendsLimit = sConfig->GetIntDefault("MaxFriendsLimit", 50);

        SendPing();
    }

    /// Deconstructor
    Habbo::~Habbo()
    {
        m_Messenger.reset();
        m_FavouriteRooms.reset();

        if (m_Socket)
        {
            if (!m_Socket->IsClosed())
                m_Socket->CloseSocket();

            m_Socket->DestroyHabbo();
        }
    }

    ///////////////////////////////////////////
    //                 ROOMS
    ///////////////////////////////////////////

    /// SetRoom
    /// @p_Room - Room Id which player is inside room
    bool Habbo::SetRoom(std::shared_ptr<Room> p_Room)
    {
        if (GetRoom())
            GetRoom()->LeaveRoom(this);

        m_Room = p_Room;
        if (auto l_Room = m_Room.lock())
            if (l_Room->EnterRoom(this))
                return true;

        return false;
    }

    /// GetRoom
    /// Get room
    std::shared_ptr<Room> Habbo::GetRoom() const
    {
        return m_Room.lock();
    }

    /// DestroyRoom
    /// Set Room to nullptr - player is no longer inside room
    void Habbo::DestroyRoom()
    {
        if (auto l_Room = m_Room.lock())
            l_Room.reset();
    }

    //////////////////////////////////////////////
    //            FAVOURITE ROOMS
    /////////////////////////////////////////////

    /// SendFavouriteRooms
    /// Send Favourite Rooms list to user
    void Habbo::SendFavouriteRooms()
    {
        HabboPacket::FavouriteRoom::FavouriteRoomResult l_Packet;
        m_FavouriteRooms->ParseSendFavouriteRooms(l_Packet.GetBuffer());
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// AddFavouriteRoom
    /// @p_IsPublic - Is room public
    /// @p_RoomId - Id of room
    void Habbo::AddFavouriteRoom(bool const& p_IsPublic, uint32 const& p_RoomId)
    {
        m_FavouriteRooms->AddFavouriteRoom(p_IsPublic, p_RoomId);
    }

    /// RemoveFavouriteRoom
    /// @p_RoomId - Id of room
    void Habbo::RemoveFavouriteRoom(uint32 const& p_RoomId)
    {
        m_FavouriteRooms->RemoveFavouriteRoom(p_RoomId);
    }

    /// SaveFavouriteRoomToDB
    /// Update Favourite Rooms to database
    void Habbo::SaveFavouriteRoomToDB()
    {
        m_FavouriteRooms->SaveToDB();
    }

    ///////////////////////////////////////////
    //           MESSENGER CONSOLE
    ///////////////////////////////////////////

    /// SendInitializeMessenger
    /// Send Initialize console on user login
    void Habbo::SendInitializeMessenger()
    {
        HabboPacket::Messenger::MessengerInitialize l_Packet;
        l_Packet.ConsoleMotto = GetConsoleMotto();
        l_Packet.FriendsLimit = sConfig->GetIntDefault("MessgengerMaxFriendsLimit", 50);
        l_Packet.ClubFriendsLimit = sConfig->GetIntDefault("MessgengerMaxFriendsClubLimit", 100);
        l_Packet.MessengerSize = m_Messenger->GetMessengerSize();
        m_Messenger->ParseMessengerInitialize(l_Packet.GetSecondaryBuffer());
        ToSocket()->SendPacket(l_Packet.Write());

        if (m_Messenger->HasFriendRequestPending())
            m_Messenger->ParseMessengerFriendRequest();

        m_Messenger->LoadMessengerMessages();
    }

    /// MessengerAcceptRequest
    /// @p_SenderId - Id of room
    void Habbo::MessengerAcceptRequest(uint32 const p_SenderId)
    {
        /// Is friend list full?
        if (m_Messenger->IsFriendListFull())
        {
            HabboPacket::Messenger::MessengerRequestBuddyError l_Packet;
            l_Packet.Error = MessengerErrorCode::FRIEND_LIST_FULL;
            ToSocket()->SendPacket(l_Packet.Write());
            return;
        }

        m_Messenger->ParseMessengerAcceptFriendRequest(p_SenderId);
    }

    /// SendMessengerUpdate
    /// Update messenger status
    void Habbo::SendMessengerUpdate()
    {
        HabboPacket::Messenger::MessengerUpdate l_Packet;
        m_Messenger->ParseMessengerUpdate(l_Packet.GetSecondaryBuffer());
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// SendSearchUserResults
    /// @p_Name - Room name user is searching for
    void Habbo::SendSearchUserResults(std::string const p_Name)
    {
        HabboPacket::Messenger::MessengerFindUserResult l_Packet;
        l_Packet.Messenger = "MESSENGER";
        m_Messenger->ParseMessengerSearchUser(l_Packet.GetSecondaryBuffer(), p_Name);
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// MessengerSendFriendRequest
    /// @p_Name - Name of friend user is sending request for
    void Habbo::MessengerSendFriendRequest(std::string const p_Name)
    {
        /// Is friend list full?
        if (m_Messenger->IsFriendListFull())
        {
            HabboPacket::Messenger::MessengerError l_Packet;
            l_Packet.Error = MessengerErrorCode::FRIEND_LIST_FULL;
            ToSocket()->SendPacket(l_Packet.Write());   
            return;
        }

        m_Messenger->ParseMessengerSendFriendRequest(p_Name);
    }

    /// MessengerRemoveFriend
   /// @p_Packet - Client packet which is being decoded
    void Habbo::MessengerRemoveFriend(std::unique_ptr<ClientPacket> p_Packet)
    {
        m_Messenger->ParseMessengerRemoveFriend(std::move(p_Packet));
    }

    /// MessengerRejectRequest
    /// @p_Packet - Client packet which is being decoded
    void Habbo::MessengerRejectRequest(std::unique_ptr<ClientPacket> p_Packet)
    {
        m_Messenger->ParseMessengerRejectRequest(std::move(p_Packet));
    }

    /// MessengerSendMessage
    /// @p_Packet - Client packet which is being decoded
    void Habbo::MessengerSendMessage(std::unique_ptr<ClientPacket> p_Packet)
    {
        m_Messenger->ParseMessengerSendMessage(std::move(p_Packet));
    }

    /// MessengerReply
    /// @p_MessageId - Id of message (account id)
    void Habbo::MessengerReply(uint32 const p_MessageId)
    {
        m_Messenger->ReadMessage(p_MessageId);
    }

    ///////////////////////////////////////////
    //             USER OBJECTS
    ///////////////////////////////////////////

    /// SendHabboObject
    /// Send user object on login
    void Habbo::SendHabboObject()
    {
        HabboPacket::HabboInfo::HabboObject l_Packet;
        l_Packet.Id             = std::to_string(m_Id);
        l_Packet.Name           = m_Name;
        l_Packet.Figure         = m_Figure;
        l_Packet.Gender         = m_Gender;
        l_Packet.Motto          = m_Motto;
        l_Packet.Tickets        = m_Tickets;
        l_Packet.PoolFigure     = m_PoolFigure;
        l_Packet.Films          = m_Films;
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// SendUpdateStatusWalk
    /// @p_X - X axis on new position
    /// @p_Y - Y axis on new position
    /// @p_Z - Z axis on new position
    StringBuffer Habbo::SendUpdateStatusWalk(int16 const p_X, int16 const p_Y, int16 const p_Z)
    {
        HabboPacket::Room::UserUpdateStatus l_Packet;
        l_Packet.GUID           = std::to_string(GetRoomGUID());
        l_Packet.CurrentX       = std::to_string(GetPositionX());
        l_Packet.CurrentY       = std::to_string(GetPositionY());
        l_Packet.CurrentZ       = std::to_string(GetPositionZ());
        l_Packet.BodyRotation   = std::to_string(GetBodyRotation());
        l_Packet.HeadRotation   = std::to_string(GetHeadRotation());
        l_Packet.Status         = GetStatus();
        l_Packet.NewX           = std::to_string(p_X);
        l_Packet.NewY           = std::to_string(p_Y);
        l_Packet.NewZ           = std::to_string(p_Z);

        /// Set our new position
        int16 l_Rotation = CalculateUserRotation(p_X, p_Y);
        UpdatePosition(p_X, p_Y, p_Z, l_Rotation);
        
        return *l_Packet.Write();
    }

    /// SendUpdateStatusStop
    /// Send Status stop when user finishes path
    StringBuffer Habbo::SendUpdateStatusStop()
    {
        SetIsWalking(false);

        HabboPacket::Room::UserUpdateStatus l_Packet;
        l_Packet.GUID           = std::to_string(GetRoomGUID());
        l_Packet.CurrentX       = std::to_string(GetPositionX());
        l_Packet.CurrentY       = std::to_string(GetPositionY());
        l_Packet.CurrentZ       = std::to_string(GetPositionZ());
        l_Packet.BodyRotation   = std::to_string(GetBodyRotation());
        l_Packet.HeadRotation   = std::to_string(GetHeadRotation());
        l_Packet.Status         = GetStatus();

        return *l_Packet.Write();
    }

    /// UpdateUserRotation
    /// @p_X - X Axis current position
    /// @p_Y - Y Axis current position
    void Habbo::UpdateUserRotation(int16 const p_X, int16 const p_Y)
    {
        int16 l_Rotation = CalculateUserRotation(p_X, p_Y);
        m_BodyRotation = l_Rotation;
        m_HeadRotation = l_Rotation;

        HabboPacket::Room::UserUpdateStatus l_Packet;
        l_Packet.GUID           = std::to_string(GetRoomGUID());
        l_Packet.CurrentX       = std::to_string(GetPositionX());
        l_Packet.CurrentY       = std::to_string(GetPositionY());
        l_Packet.CurrentZ       = std::to_string(GetPositionZ());
        l_Packet.BodyRotation   = std::to_string(GetBodyRotation());
        l_Packet.HeadRotation   = std::to_string(GetHeadRotation());
        l_Packet.Status         = GetStatus();
        
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// UpdateUserRotation
    /// @p_X - X axis on new position
    /// @p_Y - Y axis on new position
    uint8 Habbo::CalculateUserRotation(int16 const p_X, int16 const p_Y)
    {
        uint8 l_Rotation = 0;
        if (GetPositionX() > p_X && GetPositionY() > p_Y)
            l_Rotation = 7;
        else if (GetPositionX() < p_X && GetPositionY() < p_Y)
            l_Rotation = 3;
        else if (GetPositionX() > p_X && GetPositionY() < p_Y)
            l_Rotation = 5;
        else if (GetPositionX() < p_X && GetPositionY() > p_Y)
            l_Rotation = 1;
        else if (GetPositionX() > p_X)
            l_Rotation = 6;
        else if (GetPositionX() < p_X)
            l_Rotation = 2;
        else if (GetPositionY() < p_Y)
            l_Rotation = 4;
        else if (GetPositionY() > p_Y)
            l_Rotation = 0;

        return l_Rotation;
    }

    /// GetStatus
    /// Get current status on what user is doing
    std::string Habbo::GetStatus()
    {
        std::string l_Status;

        if (IsWalking())
            l_Status = "mv";

        return l_Status;
    }

    /// SendUpdateStatusWalk
    /// @p_X - X axis on new position
    /// @p_Y - Y axis on new position
    /// @p_Z - Z axis on new position
    /// @p_Rotation - New Rotation
    void Habbo::UpdatePosition(int16 const& p_X, int16 const& p_Y, int16 const& p_Z, int16 const& p_O)
    {
        m_PositionX = p_X;
        m_PositionY = p_Y;
        m_PositionZ = p_Z;
        m_BodyRotation = p_O;
        m_HeadRotation = p_O;
    }

    ///////////////////////////////////////////
    //             ACCOUNT INFO
    ///////////////////////////////////////////

    /// SendAccountPreferences
    /// Send user account preferences (set from users.account database)
    void Habbo::SendAccountPreferences()
    {
        HabboPacket::HabboInfo::AccountPreferences l_Packet;
        l_Packet.SoundEnabled = IsSoundEnabled();
        l_Packet.TutorialFinished = true; ///< TODO
        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// SendAccountBadges
    /// Send user account badges (set from users.account_badges database)
    void Habbo::SendAccountBadges()
    {
        HabboPacket::HabboInfo::AvailableBadges l_Packet;

        for (auto const& itr : m_Badges)
        {
            if (itr.IsActive())
                l_Packet.ActiveBadges++;

            l_Packet.Badges.push_back(itr.GetBadge());
        }

        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// SendFuseRights
    /// Send user account rights (set from users.rank_fuserights database)
    void Habbo::SendFuseRights()
    {
        HabboPacket::HabboInfo::FuseRights l_Packet;

        for (auto const& itr : m_FuseRights)
            l_Packet.HabboFuseRights.push_back(itr.GetFuseRight());

        ToSocket()->SendPacket(l_Packet.Write());
    }

    /// SendClubStatus
    /// Send user account club status (set from users.club_subscriptions database)
    void Habbo::SendClubStatus()
    {
        // TODO;
    }

    ///////////////////////////////////////////
    //             HABBO INFO
    ///////////////////////////////////////////

    /// InitializeHabboData
    /// Initialize Habbo Info data when user logs in
    void Habbo::InitializeHabboData()
    {
        m_Messenger = std::make_unique<Messenger>(this);
        m_Messenger->UpdateConsole();

        m_FavouriteRooms = std::make_unique<FavouriteRoom>(m_Id);
        m_FavouriteRooms->LoadFavouriteRooms();
    }

    /// Update
    /// Update Habbo user
    /// @p_Diff - Tick Timer
    bool Habbo::Update(uint32 const& p_Diff)
    {
        std::lock_guard<std::mutex> l_Guard(m_Mutex);

        if (m_Socket && !m_Socket->IsClosed())
        {
            if (m_UpdateAccount < p_Diff)
            {
                QueryDatabase database("users");
                database.PrepareQuery("UPDATE account SET email = ?, figure = ?, pool_figure = ?, motto = ?, console_motto = ?, birthday = ?, gender = ?, credits = ?, tickets = ?, films = ?, sound_enabled = ?");
                database.GetStatement()->setString(1, GetEmail());
                database.GetStatement()->setString(2, GetFigure());
                database.GetStatement()->setString(3, GetPoolFigure());
                database.GetStatement()->setString(4, GetMotto());
                database.GetStatement()->setString(5, GetConsoleMotto());
                database.GetStatement()->setString(6, GetBirthday());
                database.GetStatement()->setString(7, GetGender());
                database.GetStatement()->setUInt(8, GetCredits());
                database.GetStatement()->setUInt(9, GetTickets());
                database.GetStatement()->setUInt(10, GetFilms());
                database.GetStatement()->setBoolean(11, IsSoundEnabled());
                database.ExecuteQuery();

                m_UpdateAccount = sConfig->GetIntDefault("PlayerAccountUpdate", 600000);
            }
            else
                m_UpdateAccount -= p_Diff;

            if (m_PingInterval < p_Diff)
            {
                if (IsPonged())
                {
                    SendPing();
                    m_PingInterval = sConfig->GetIntDefault("PongInterval", 10000);
                }
                else
                {
                    Logout(LOGOUT_TIMEOUT);
                    LOG_INFO << "Disconnecting Habbo: " << GetId() << " have not recieved a pong back";
                    return false;
                }
            }
            else
                m_PingInterval -= p_Diff;

            return true;
        }
        else
            return false;
    }

    /// Logout
    /// @p_Reason - Logout reason (enum LogoutReason)
    void Habbo::Logout(LogoutReason const p_Reason /*= LOGGED_OUT*/)
    {
        SaveFavouriteRoomToDB();

        if (GetRoom())
            GetRoom()->LeaveRoom(this);

        HabboPacket::Misc::HotelLogout l_Packet;
        l_Packet.Reason = p_Reason;
        ToSocket()->SendPacket(l_Packet.Write());

        if (m_Socket)
        {
            if (!m_Socket->IsClosed())
                m_Socket->CloseSocket();

            m_Socket->DestroyHabbo();
        }
    }

    /// SendPing
    /// Send Ping response to client
    void Habbo::SendPing()
    {
        m_Ponged = false;
        HabboPacket::Misc::Ping l_Packet;
        ToSocket()->SendPacket(l_Packet.Write());
    }

} ///< NAMESPACE STEERSTONE