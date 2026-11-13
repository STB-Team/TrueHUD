#pragma once
#include "TrueHUDAPI.h"
#include "Widgets/UpdateCache.h"

namespace Scaleform
{
	class InfoBarBase : public TRUEHUD_API::WidgetBase
	{
	public:
		enum TargetType : std::uint8_t
		{
			kTarget = 0,
			kEnemy = 1,
			kTeammate = 2,
			kOther = 3,
		};

		enum TargetState : std::uint8_t
		{
			kAlive = 0,
			kStunned = 1,
			kBleedingOut = 2,
			kBleedingOutEssential = 3,
			kDead = 4
		};

		enum WidgetStateMode : std::uint8_t
		{
			kAdd = 0,
			kShow = 1,
			kHide = 2,
			kRemove = 3,
			kTargetKilled = 4
		};

		enum BarType : std::uint8_t
		{
			kActorInfoBar = 0,
			kBossInfoBar = 1
		};

		InfoBarBase(RE::GPtr<RE::GFxMovieView> a_view, RE::ObjectRefHandle a_refHandle) :
			WidgetBase(a_view, a_refHandle.native_handle()),
			_refHandle(a_refHandle)
		{}

		virtual void Initialize() override;
		virtual void Dispose() override;

		virtual void SetWidgetState(WidgetStateMode a_state);
		virtual void FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong);
		virtual void FlashSpecial(bool a_bLong);
		virtual void RefreshColors();

		RE::ObjectRefHandle _refHandle;

	protected:
		virtual void UpdatePosition() = 0;
		virtual void LoadConfig() = 0;

		virtual void UpdateInfo(float a_deltaTime);

		// Refreshes the data that doesn't need to be recalculated every single frame.
		// Everything gathered here is either constant for the actor's lifetime or slow
		// enough that a few frames of latency can't be seen, while being expensive to
		// query (line of sight checks, active effect list walks, keyword lookups).
		void RefreshCachedInfo(RE::Actor* a_actor, RE::PlayerCharacter* a_playerCharacter, bool a_bIsTarget);

		bool _bIsOffscreen = false;

		BarType _barType;

		// Cached results of the expensive queries, refreshed by RefreshCachedInfo.
		// Negative timer forces a refresh on the first update.
		float _cachedInfoTimer = -1.f;
		std::string _cachedName;
		std::string _cachedRegenString;
		uint16_t _cachedLevel = 0;
		uint32_t _cachedSoulType = 0;
		bool _cachedIsBoss = false;
		bool _cachedIsHostile = false;
		bool _cachedIsTeammate = false;
		bool _cachedHasLineOfSight = true;

		// loadConfig initializes the movie on its next Advance, not during the call.
		bool _flashInitialized = false;
		UpdateCache<17, 2> _dataCache;
		UpdateCache<3> _specialCache;
	};
}
