#include "stdafx.h"
#include "DBAgent.h"

void CGameServerDlg::SendEventRemainingTime(bool bSendAll, CUser *pUser, uint8_t ZoneID)
{
	Packet result(WIZ_BIFROST);
	uint16_t nRemainingTime = 0;

	if (ZoneID == ZONE_BATTLE4)
		nRemainingTime = m_byBattleRemainingTime / 2;
	else if (ZoneID == ZONE_BIFROST || ZoneID ==  ZONE_RONARK_LAND)
		nRemainingTime = m_sBifrostRemainingTime;

	result << uint8_t(2) << nRemainingTime;

	if (pUser)
		pUser->Send(&result);
	else if (bSendAll)
	{
		if (ZoneID == ZONE_BATTLE4)
			Send_All(&result,nullptr, 0, ZONE_BATTLE4);
		else
		{
			Send_All(&result,nullptr, 0, ZONE_RONARK_LAND);
			Send_All(&result,nullptr, 0, ZONE_BIFROST);
		}
	}
}

void CUser::BifrostProcess(CUser * pUser)
{
	if (pUser == nullptr)
		return;

	if (g_pMain->m_BifrostVictory == 0 && g_pMain->m_bAttackBifrostMonument)
	{
		g_pMain->m_sBifrostTime = g_pMain->m_xBifrostTime;
		g_pMain->m_sBifrostRemainingTime = g_pMain->m_sBifrostTime;
		g_pMain->m_BifrostVictory = pUser->GetNation();
		g_pMain->SendFormattedResource(pUser->GetNation() == ELMORAD ? IDS_BEEF_ROAST_VICTORY_ELMORAD : IDS_BEEF_ROAST_VICTORY_KARUS, Nation::ALL,false);
		g_pMain->SendEventRemainingTime(true, nullptr, ZONE_BIFROST);

		if (g_pMain->m_bAttackBifrostMonument)
			g_pMain->m_bAttackBifrostMonument = false;
	}
	else if (g_pMain->m_BifrostVictory == 1 || g_pMain->m_BifrostVictory == 2) 
	{
		if (pUser->GetNation() != g_pMain->m_BifrostVictory && g_pMain->m_bAttackBifrostMonument)
		{
			g_pMain->m_BifrostVictory = 3;
			g_pMain->SendFormattedResource(pUser->GetNation() == ELMORAD ? IDS_BEEF_ROAST_VICTORY_ELMORAD : IDS_BEEF_ROAST_VICTORY_KARUS, Nation::ALL,false);

			if (g_pMain->m_bAttackBifrostMonument)
				g_pMain->m_bAttackBifrostMonument = false;
		}
	}
}

void CUser::BorderDefanceWarProcess(CUser* pUser)
{
	if (pUser == nullptr)
		return;

	if (pUser->GetZoneID() != ZONE_BORDER_DEFENSE_WAR)
		return;

	if (pUser->GetEventRoom() < 1)
		return;

	if (pUser->GetNation() == ELMORAD) {
		g_pMain->pTempleEvent.KarusDeathCount[pUser->GetEventRoom()] += 61;
	}
	else {
		g_pMain->pTempleEvent.ElmoDeathCount[pUser->GetEventRoom()] += 61;
	}

	Packet resultmer;
	std::string bufferpro;

	if (GetNation() == 1)
		bufferpro = string_format("[Event Message] Border Defance War finished. Karus nation has won. You will teleport in 20 seconds.", g_pMain->pTempleEvent.ElMoradUserCount, g_pMain->pTempleEvent.KarusUserCount, g_pMain->m_nTempleEventRemainSeconds);
	else
		bufferpro = string_format("[Event Message] Border Defance War finished. Human nation has won. You will teleport in 20 seconds.", g_pMain->pTempleEvent.ElMoradUserCount, g_pMain->pTempleEvent.KarusUserCount, g_pMain->m_nTempleEventRemainSeconds);
	ChatPacket::Construct(&resultmer, 7, &bufferpro);

	
	g_pMain->Send_All(&resultmer, nullptr, Nation::ALL, GetEventRoom());

	g_pMain->TempleEventFinish(0);


}
void CUser::CastleSiegeWarProcess(CUser * pUser)
{
	if (pUser == nullptr)
		return;

	_KNIGHTS_SIEGE_WARFARE *pKnightSiegeWar = g_pMain->GetSiegeMasterKnightsPtr(1);
	CKnights * pKnights = g_pMain->GetClanPtr(pUser->m_bKnights);

	pKnightSiegeWar->sMasterKnights = pKnights->m_sIndex;

	g_pMain->UpdateSiege(pKnightSiegeWar->sCastleIndex, pKnightSiegeWar->sMasterKnights, pKnightSiegeWar->bySiegeType, pKnightSiegeWar->byWarDay, pKnightSiegeWar->byWarTime, pKnightSiegeWar->byWarMinute);
	g_pMain->m_KnightsSiegeWarfareArray.GetData(pKnightSiegeWar->sMasterKnights);

	g_pMain->m_SiegeWarWinKnightsNotice = pKnights->GetName();
	g_pMain->Announcement(IDS_NPC_GUIDON_DESTORY);
	g_pMain->m_byBattleSiegeWarMomument = true;
	g_pMain->KickOutZoneUsers(ZONE_DELOS, ZONE_DELOS);
	if (pKnightSiegeWar != nullptr)
	{
		Packet result(WIZ_SIEGE);
		result << uint8_t(2) << pKnights->GetID() << pKnights->m_sMarkVersion;
		g_pMain->Send_Zone(&result,ZONE_DELOS);
	}
}

void CUser::TempleProcess(Packet& pkt)
{
	uint8_t opcode = pkt.read<uint8_t>();

	switch (opcode)
	{
	case TEMPLE_EVENT_JOIN:
		TempleOperations(opcode);
		break;
	case TEMPLE_EVENT_DISBAND:
		TempleOperations(opcode);
		break;
	}
}

void CUser::TempleOperations(uint8_t bType)
{
	uint16_t nActiveEvent = (uint16_t)g_pMain->pTempleEvent.ActiveEvent;

	if (nActiveEvent != TEMPLE_EVENT_BORDER_DEFENCE_WAR)
		return;

	// =========================================================
	// JOIN
	// =========================================================
	if (bType == TEMPLE_EVENT_JOIN && !isEventUser())
	{
		if (GetNation() == KARUS)
			g_pMain->pTempleEvent.KarusUserCount++;
		else
			g_pMain->pTempleEvent.ElMoradUserCount++;

		g_pMain->pTempleEvent.AllUserCount =
			g_pMain->pTempleEvent.KarusUserCount +
			g_pMain->pTempleEvent.ElMoradUserCount;

		g_pMain->AddEventUser(this);

		printf(
			"[BDW] JOIN %s | Human=%d Karus=%d All=%d\n",
			GetName().c_str(),
			g_pMain->pTempleEvent.ElMoradUserCount,
			g_pMain->pTempleEvent.KarusUserCount,
			g_pMain->pTempleEvent.AllUserCount
		);

		// -----------------------------------------------------
		// CLIENT'A JOIN BAŞARILI CEVABI
		// -----------------------------------------------------
		Packet result(WIZ_EVENT);

		result << uint8_t(TEMPLE_EVENT_JOIN);
		result << uint8_t(1);
		result << uint16_t(nActiveEvent);

		Send(&result);

		// -----------------------------------------------------
		// BDW KONTENJAN / SAYI GÜNCELLEMESİ
		// -----------------------------------------------------
		g_pMain->TemplEventBDWSendJoinScreenUpdate(this);

		return;
	}

	// =========================================================
	// DISBAND
	// =========================================================
	if (bType == TEMPLE_EVENT_DISBAND && isEventUser())
	{
		if (GetNation() == KARUS)
		{
			if (g_pMain->pTempleEvent.KarusUserCount > 0)
				g_pMain->pTempleEvent.KarusUserCount--;
		}
		else
		{
			if (g_pMain->pTempleEvent.ElMoradUserCount > 0)
				g_pMain->pTempleEvent.ElMoradUserCount--;
		}

		g_pMain->pTempleEvent.AllUserCount =
			g_pMain->pTempleEvent.KarusUserCount +
			g_pMain->pTempleEvent.ElMoradUserCount;

		g_pMain->RemoveEventUser(this);

		printf(
			"[BDW] DISBAND %s | Human=%d Karus=%d All=%d\n",
			GetName().c_str(),
			g_pMain->pTempleEvent.ElMoradUserCount,
			g_pMain->pTempleEvent.KarusUserCount,
			g_pMain->pTempleEvent.AllUserCount
		);

		// -----------------------------------------------------
		// CLIENT'A DISBAND BAŞARILI CEVABI
		// -----------------------------------------------------
		Packet result(WIZ_EVENT);

		result << uint8_t(TEMPLE_EVENT_DISBAND);
		result << uint8_t(1);
		result << uint16_t(nActiveEvent);

		Send(&result);

		// -----------------------------------------------------
		// BDW KONTENJAN / SAYI GÜNCELLEMESİ
		// -----------------------------------------------------
		g_pMain->TemplEventBDWSendJoinScreenUpdate(this);

		return;
	}
}

void CGameServerDlg::AddEventUser(CUser* pUser)
{
	if (pUser == nullptr)
	{
		TRACE("#### AddEventUser : pUser == nullptr ####\n");
		return;
	}

	_TEMPLE_EVENT_USER* pEventUser = new _TEMPLE_EVENT_USER;

	_TEMPLE_EVENT_USER* pEventUserControl = g_pMain->m_TempleEventUserArray.GetData(pUser->GetSocketID());

	if (pEventUserControl != nullptr && pEventUserControl->m_bIsFinished == true)
	{
		pEventUserControl->m_bEventRoom = pUser->GetEventRoom();
		pEventUserControl->m_bIsFinished = false;
		pEventUserControl->m_bIsnewRegister = false;
		return;
	}


	pEventUser->m_socketID = pUser->GetSocketID();
	pEventUser->m_bEventRoom = pUser->GetEventRoom();
	pEventUser->m_bIsFinished = false;
	pEventUser->m_bIsnewRegister = true;

	if (!g_pMain->m_TempleEventUserArray.PutData(pEventUser->m_socketID, pEventUser))
		delete pEventUser;
}



void CGameServerDlg::RemoveEventUser(CUser* pUser)
{
	if (pUser == nullptr)
	{
		TRACE("#### RemoveEventUser : pUser == nullptr ####\n");
		return;
	}

	if (g_pMain->m_TempleEventUserArray.GetData(pUser->GetSocketID()) != nullptr)
		g_pMain->m_TempleEventUserArray.DeleteData(pUser->GetSocketID());



}

void CGameServerDlg::UpdateEventUser(CUser* pUser, uint16_t nEventRoom)
{
	if (pUser == nullptr)
	{
		TRACE("#### UpdateEventUser : pUser == nullptr ####\n");
		return;
	}

	_TEMPLE_EVENT_USER* pEventUser = g_pMain->m_TempleEventUserArray.GetData(pUser->GetSocketID());



	if (pEventUser)
	{
		pEventUser->m_bEventRoom = nEventRoom;
		pUser->m_bEventRoom = nEventRoom;
		pUser->SetUserEventRoom(nEventRoom);
		pUser->SetUnitEventRoom(nEventRoom);
		pEventUser->m_bIsFinished = false;
		pEventUser->m_bIsnewRegister = false;
	}
}

bool CUser::isEventUser()
{
	_TEMPLE_EVENT_USER* pEventUser = g_pMain->m_TempleEventUserArray.GetData(GetSocketID());

	if (pEventUser != nullptr && (pEventUser->m_bIsFinished == true || pEventUser->m_bIsnewRegister))
		return true;

	return false;
}

uint8_t CUser::GetMonsterChallengeTime() 
{ 
	if (g_pMain->m_bForgettenTempleIsActive
		&& g_pMain->m_nForgettenTempleLevelMin != 0 
		&& g_pMain->m_nForgettenTempleLevelMax != 0
		&& GetLevel() >= g_pMain->m_nForgettenTempleLevelMin 
		&& GetLevel() <= g_pMain->m_nForgettenTempleLevelMax
		&& !g_pMain->m_bForgettenTempleSummonMonsters)
		return g_pMain->m_nForgettenTempleChallengeTime; 

	return 0;
}

uint8_t CUser::GetMonsterChallengeUserCount() { return g_pMain->m_nForgettenTempleUsers.size(); }
