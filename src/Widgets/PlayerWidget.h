#pragma once
#include "Widgets/ShoutIndicator.h"

namespace Scaleform
{
	class PlayerWidget : public ShoutIndicator
	{
	public:
		PlayerWidget(RE::GPtr<RE::GFxMovieView> a_view) :
			ShoutIndicator(a_view)
		{}

		virtual void Update(float a_deltaTime) override;
		virtual void Initialize() override;
		virtual void Dispose() override;

		virtual void FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong);
		virtual void FlashSpecial(bool a_bLong);
		virtual void RefreshColors();
		virtual void UpdatePenaltyPercentages();
		virtual void UpdatePlayerWidgetChargeMeters(float a_percent, bool a_bForce, bool a_bLeftHand, bool a_bShow);

	protected:
		virtual void UpdatePosition() override;
		virtual void LoadConfig() override;

		virtual void UpdateInfo(float a_deltaTime);

		virtual void UpdatePenalty();

		// Walking the player's active effect list to sum up the regeneration rates is far
		// too expensive to do three times a frame, and the numbers can't be read that fast
		// anyway. Negative timer forces a refresh on the first update.
		void RefreshRegenStrings();
		float _regenTimer = -1.f;
		std::string _healthRegenString;
		std::string _magickaRegenString;
		std::string _staminaRegenString;

		virtual void ToggleVanillaBars(bool a_bEnable);
		virtual void ToggleVanillaEnchantmentChargeMeters(bool a_bEnable);

		bool _bInitialized = false;
		float _healthPenaltyPct = 0.f;
		float _magickaPenaltyPct = 0.f;
		float _staminaPenaltyPct = 0.f;

		bool _bVanillaBarsHidden = false;
		bool _bVanillaEnchantmentChargeMetersHidden = false;
	};
}
