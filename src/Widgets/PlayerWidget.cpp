#include "Widgets/PlayerWidget.h"
#include "Offsets.h"
#include "Settings.h"
#include "Utils.h"
#include "HUDHandler.h"
#include "Perf.h"

namespace Scaleform
{
	void PlayerWidget::Update(float a_deltaTime)
	{
		Perf::Scope scope(Perf::Phase::Player);
		_bCacheReady = _bInitialized;
		if (!_bInitialized) {
			RE::GFxValue result;
			Perf::Invoke(_object, "isInitialized", &result);
			if (result.GetBool()) {
				_bInitialized = true;
				UpdatePenalty();
			}
		}

		UpdateInfo(a_deltaTime);

		if (Settings::bPlayerWidgetHideVanillaBars && Settings::bPlayerWidgetForceHideVanillaBars) {
			ToggleVanillaBars(false);
		}

		if (Settings::bPlayerWidgetDisplayEnchantmentChargeMeter && Settings::bPlayerWidgetForceHideVanillaBars) {
			ToggleVanillaEnchantmentChargeMeters(false);
		}
	}

	void PlayerWidget::Initialize()
	{
		LoadConfig();

		if (Settings::bPlayerWidgetHideVanillaBars) {
			ToggleVanillaBars(false);
		} else if (_bVanillaBarsHidden) {
			ToggleVanillaBars(true);
		}

		if (Settings::bPlayerWidgetDisplayEnchantmentChargeMeter) {
			ToggleVanillaEnchantmentChargeMeters(false);
		} else if (_bVanillaEnchantmentChargeMetersHidden) {
			ToggleVanillaEnchantmentChargeMeters(true);
		}

		UpdatePosition();
	}

	void PlayerWidget::Dispose()
	{
		if (_bVanillaBarsHidden) {
			ToggleVanillaBars(true);
		}

		if (_bVanillaEnchantmentChargeMetersHidden) {
			ToggleVanillaEnchantmentChargeMeters(true);
		}

		_object.Invoke("cleanUp", nullptr, nullptr, 0);
	}

	void PlayerWidget::FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong)
	{
		RE::GFxValue args[2];
		args[0].SetNumber(static_cast<int32_t>(a_actorValue));
		args[1].SetBoolean(a_bLong);

		_object.Invoke("flashAV", nullptr, args, 2);
	}

	void PlayerWidget::FlashSpecial(bool a_bLong)
	{
		RE::GFxValue arg;
		arg.SetBoolean(a_bLong);

		_object.Invoke("flashSpecial", nullptr, &arg, 1);
	}

	void PlayerWidget::RefreshColors()
	{
		using ColorType = TRUEHUD_API::BarColorType;
		
		auto hudMenu = HUDHandler::GetTrueHUDMenu();
		auto playerHandle = RE::PlayerCharacter::GetSingleton()->GetHandle();

		RE::GFxValue args[20];
		args[0].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BarColor));
		args[1].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PhantomColor));
		args[2].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BackgroundColor));
		args[3].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PenaltyColor));
		args[4].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::FlashColor));
		args[5].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BarColor));
		args[6].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PhantomColor));
		args[7].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BackgroundColor));
		args[8].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PenaltyColor));
		args[9].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::FlashColor));
		args[10].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BarColor));
		args[11].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PhantomColor));
		args[12].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BackgroundColor));
		args[13].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PenaltyColor));
		args[14].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::FlashColor));
		args[15].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BarColor));
		args[16].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PhantomColor));
		args[17].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BackgroundColor));
		args[18].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PenaltyColor));
		args[19].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::FlashColor));
		_object.Invoke("setColors", nullptr, args, 20);
	}

	void PlayerWidget::UpdatePenaltyPercentages()
	{
		if (Settings::glob_survivalModeEnabled &&
			Settings::glob_survivalHealthPenaltyPercent &&
			Settings::glob_survivalMagickaPenaltyPercent &&
			Settings::glob_survivalStaminaPenaltyPercent)
		{
			if (Settings::glob_survivalModeEnabled->value == 1) {
				_healthPenaltyPct = Settings::glob_survivalHealthPenaltyPercent->value / 100.f;
				_magickaPenaltyPct = Settings::glob_survivalMagickaPenaltyPercent->value / 100.f;
				_staminaPenaltyPct = Settings::glob_survivalStaminaPenaltyPercent->value / 100.f;
			} else {
				_healthPenaltyPct = 0.f;
				_magickaPenaltyPct = 0.f;
				_staminaPenaltyPct = 0.f;
			}
			UpdatePenalty();
		}
	}

	void PlayerWidget::UpdatePlayerWidgetChargeMeters(float a_percent, bool a_bForce, bool a_bLeftHand, bool a_bShow)
	{
		RE::GFxValue args[4];
		args[0].SetNumber(a_percent);
		args[1].SetBoolean(a_bForce);
		args[2].SetBoolean(a_bLeftHand);
		args[3].SetBoolean(a_bShow);
		_object.Invoke("updateEnchantmentChargeMeter", nullptr, args, 4);
	}

	void PlayerWidget::UpdatePosition()
	{
		RE::GFxValue::DisplayInfo displayInfo;
		float scale = Settings::fPlayerWidgetScale * 100.f;
		displayInfo.SetScale(scale, scale);

		if (Settings::bPlayerWidgetCombined) {
			RE::GRectF rect = _view->GetVisibleFrameRect();

			RE::NiPoint2 screenPos;

			auto def = _view->GetMovieDef();
			if (def) {
				screenPos.x = def->GetWidth();
				screenPos.y = def->GetHeight();
			}

			screenPos.x *= Settings::fPlayerWidgetX;
			screenPos.y *= Settings::fPlayerWidgetY;

			displayInfo.SetPosition(screenPos.x, screenPos.y);
		} else {
			displayInfo.SetPosition(0, 0);
		}
		
		_object.SetDisplayInfo(displayInfo);
	}
	void PlayerWidget::RefreshRegenStrings()
	{
		Perf::Scope scope(Perf::Phase::PlayerRegen);
		float health = 0.f;
		float magicka = 0.f;
		float stamina = 0.f;

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto active_effects = player ? player->AsMagicTarget()->GetActiveEffectList() : nullptr;
		if (active_effects) {
			// One pass over the list for all three actor values instead of one pass each.
			for (auto active_effect : *active_effects) {
				const auto av = active_effect->effect->baseEffect->data.primaryAV;
				if (av != RE::ActorValue::kHealth && av != RE::ActorValue::kMagicka && av != RE::ActorValue::kStamina) {
					continue;
				}

				if (active_effect->flags.any(RE::ActiveEffect::Flag::kInactive)) {
					continue;
				}

				const auto baseEffect = active_effect->effect->baseEffect;
				if (!baseEffect->HasKeywordString("aaMZkw_RestoreStaminaRace") &&
					!baseEffect->HasKeywordString("aaMZkw_RestoreHealthRace") &&
					!baseEffect->HasKeywordString("aaMZkw_RestoreMagickaRace")) {
					continue;
				}

				switch (av) {
				case RE::ActorValue::kHealth:
					health += active_effect->magnitude;
					break;
				case RE::ActorValue::kMagicka:
					magicka += active_effect->magnitude;
					break;
				case RE::ActorValue::kStamina:
					stamina += active_effect->magnitude;
					break;
				default:
					break;
				}
			}
		}

		_healthRegenString = fmt::format("{:.{}f}"sv, health, 2);
		_magickaRegenString = fmt::format("{:.{}f}"sv, magicka, 2);
		_staminaRegenString = fmt::format("{:.{}f}"sv, stamina, 2);
	}
	void PlayerWidget::LoadConfig()
	{
		_bInitialized = false;
		_bCacheReady = false;
		_specialCache.Reset();
		_penaltyCache.Reset();
		using ColorType = TRUEHUD_API::BarColorType;

		auto hudHandler = HUDHandler::GetSingleton();
		auto hudMenu = HUDHandler::GetTrueHUDMenu();

		auto playerHandle = RE::PlayerCharacter::GetSingleton()->GetHandle();

		RE::NiPoint2 stageSize;
		auto def = _view->GetMovieDef();
		if (def) {
			stageSize.x = def->GetWidth();
			stageSize.y = def->GetHeight();
		}

		RE::GFxValue args[54];
		args[0].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetHealthBarDirection));
		args[1].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetMagickaBarDirection));
		args[2].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetStaminaBarDirection));
		args[3].SetBoolean(Settings::bPlayerWidgetDisplayPhantomBars);
		args[4].SetBoolean(Settings::bPlayerWidgetDisplaySpecialPhantomBar);
		args[5].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetHealthMode));
		args[6].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetMagickaMode));
		args[7].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetStaminaMode));
		args[8].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetShoutIndicatorMode));
		args[9].SetNumber(static_cast<uint32_t>(Settings::uPlayerWidgetSpecialMode));
		args[10].SetBoolean(hudHandler->bSpecialMode);
		args[11].SetBoolean(Settings::bPlayerWidgetDisplayEnchantmentChargeMeter);
		args[12].SetNumber(Settings::fPlayerWidgetPhantomDuration);
		args[13].SetNumber(Settings::fPlayerWidgetScale);
		args[14].SetNumber((Settings::bPlayerWidgetUseHUDOpacity ? *g_fHUDOpacity : Settings::fPlayerWidgetOpacity) * 100.f);
		args[15].SetNumber(Settings::fPlayerWidgetBarWidth);
		args[16].SetNumber(Settings::fPlayerWidgetSpecialBarWidth);
		args[17].SetBoolean(Settings::bPlayerWidgetScaleBars);
		args[18].SetNumber(Settings::fPlayerWidgetMinBarWidth);
		args[19].SetNumber(Settings::fPlayerWidgetMaxBarWidth);
		args[20].SetNumber(Settings::fPlayerWidgetHealthBarScaleMult);
		args[21].SetNumber(Settings::fPlayerWidgetMagickaBarScaleMult);
		args[22].SetNumber(Settings::fPlayerWidgetStaminaBarScaleMult);
		args[23].SetNumber(stageSize.x * Settings::fPlayerWidgetX);
		args[24].SetNumber(stageSize.y * Settings::fPlayerWidgetY);
		args[25].SetNumber(stageSize.x * Settings::fPlayerWidgetEnchantmentChargeMeterX);
		args[26].SetNumber(stageSize.y * Settings::fPlayerWidgetEnchantmentChargeMeterY);
		args[27].SetNumber(Settings::bPlayerWidgetCombined);
		args[28].SetNumber(stageSize.x * Settings::fPlayerWidgetHealthX);
		args[29].SetNumber(stageSize.y * Settings::fPlayerWidgetHealthY);
		args[30].SetNumber(stageSize.x * Settings::fPlayerWidgetMagickaX);
		args[31].SetNumber(stageSize.y * Settings::fPlayerWidgetMagickaY);
		args[32].SetNumber(stageSize.x * Settings::fPlayerWidgetStaminaX);
		args[33].SetNumber(stageSize.y * Settings::fPlayerWidgetStaminaY);
		args[34].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BarColor));
		args[35].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PhantomColor));
		args[36].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::BackgroundColor));
		args[37].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::PenaltyColor));
		args[38].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kHealth, ColorType::FlashColor));
		args[39].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BarColor));
		args[40].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PhantomColor));
		args[41].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::BackgroundColor));
		args[42].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::PenaltyColor));
		args[43].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kMagicka, ColorType::FlashColor));
		args[44].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BarColor));
		args[45].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PhantomColor));
		args[46].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::BackgroundColor));
		args[47].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::PenaltyColor));
		args[48].SetNumber(hudMenu->GetBarColor(playerHandle, RE::ActorValue::kStamina, ColorType::FlashColor));
		args[49].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BarColor));
		args[50].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PhantomColor));
		args[51].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::BackgroundColor));
		args[52].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::PenaltyColor));
		args[53].SetNumber(hudMenu->GetSpecialBarColor(playerHandle, ColorType::FlashColor));
		_object.Invoke("loadConfig", nullptr, args, 54);
	}

	void PlayerWidget::UpdateInfo(float a_deltaTime)
	{
		SetPercent(GetPercent());
		UpdatePenaltyPercentages();

		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		float permanentHealth, temporaryHealth, maxHealth, health, permanentMagicka, temporaryMagicka, maxMagicka, magicka, permanentStamina, temporaryStamina, maxStamina, stamina;

		auto actorValueOwner = playerCharacter->AsActorValueOwner();

		permanentHealth = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kHealth);
		temporaryHealth = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kHealth);
		maxHealth = permanentHealth + temporaryHealth;
		maxHealth /= (1.f - _healthPenaltyPct);
		if (playerCharacter->IsDead()) {
			health = 0;
		} else {
			health = actorValueOwner->GetActorValue(RE::ActorValue::kHealth);
		}

		permanentMagicka = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kMagicka);
		temporaryMagicka = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kMagicka);
		maxMagicka = permanentMagicka + temporaryMagicka;
		maxMagicka /= (1.f - _magickaPenaltyPct);
		magicka = actorValueOwner->GetActorValue(RE::ActorValue::kMagicka);

		RE::ActorPtr mount;
		bool bIsInMountMode = false;
		if (Settings::bPlayerWidgetDisplayMountStamina && playerCharacter->GetMount(mount)) {
			auto mountActorValueOwner = mount->AsActorValueOwner();

			permanentStamina = mountActorValueOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
			temporaryStamina = mount->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);
			maxStamina = permanentStamina + temporaryStamina;
			stamina = mountActorValueOwner->GetActorValue(RE::ActorValue::kStamina);
			bIsInMountMode = true;
		} else {
			permanentStamina = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
			temporaryStamina = playerCharacter->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);
			maxStamina = permanentStamina + temporaryStamina;
			maxStamina /= (1.f - _staminaPenaltyPct);
			stamina = actorValueOwner->GetActorValue(RE::ActorValue::kStamina);
			bIsInMountMode = false;
		}
		bool bIsInCombat = playerCharacter->IsInCombat();

		_regenTimer -= a_deltaTime;
		if (_regenTimer < 0.f) {
			RefreshRegenStrings();
			_regenTimer = Settings::fWidgetSlowUpdateInterval;
		}

		RE::GFxValue args[16];
		args[0].SetNumber(ceil(maxHealth));
		args[1].SetNumber(ceil(health));
		args[2].SetNumber(Utils::GetPct(health, maxHealth));
		args[3].SetNumber(ceil(maxMagicka));
		args[4].SetNumber(ceil(magicka));
		args[5].SetNumber(Utils::GetPct(magicka, maxMagicka));
		args[6].SetNumber(ceil(maxStamina));
		args[7].SetNumber(ceil(stamina));
		args[8].SetNumber(Utils::GetPct(stamina, maxStamina));
		args[9].SetBoolean(bIsInCombat);
		args[10].SetBoolean(bIsInMountMode);
		args[11].SetBoolean(false);
		args[12].SetNumber(Settings::ManaLock->value);
		args[13].SetString(_healthRegenString);
		args[14].SetString(_staminaRegenString);
		args[15].SetString(_magickaRegenString);
		// This call also refreshes combat visibility timers and animated text placement.
		// Equal resource values alone do not make it safe to omit.
		Perf::Invoke(_object, "updateData", nullptr, args, 16);
		Perf::Count(Perf::Counter::DataSent);

		auto hudHandler = HUDHandler::GetSingleton();
		if (hudHandler && hudHandler->bSpecialFunctionsProvided && hudHandler->bDisplaySpecialForPlayer) {
			auto maxSpecial = fmax(hudHandler->GetMaxSpecial(playerCharacter), 0.f);
			auto currentSpecial = fmax(hudHandler->GetCurrentSpecial(playerCharacter), 0.f);
			const UpdateCache<3>::Numbers special{ ceil(maxSpecial), ceil(currentSpecial), Utils::GetPct(currentSpecial, maxSpecial) };
			const bool sendSpecial = !Perf::UseOptimizations() || !_bCacheReady || !_specialCache.Matches(special);
			if (sendSpecial) {
				RE::GFxValue specialArgs[3];
				for (std::size_t i = 0; i < special.size(); ++i) {
					specialArgs[i].SetNumber(special[i]);
				}
				if (Perf::Invoke(_object, "updateSpecial", nullptr, specialArgs, 3) && _bCacheReady) {
					_specialCache.Store(special);
				}
			}
			Perf::Count(sendSpecial ? Perf::Counter::SpecialSent : Perf::Counter::SpecialSkipped);
		}
	}

	void PlayerWidget::UpdatePenalty()
	{
		const UpdateCache<3>::Numbers penalty{ _healthPenaltyPct, _magickaPenaltyPct, _staminaPenaltyPct };
		if (Perf::UseOptimizations() && _bCacheReady && _penaltyCache.Matches(penalty)) {
			Perf::Count(Perf::Counter::PenaltySkipped);
			return;
		}

		RE::GFxValue penaltyArgs[4];
		for (std::size_t i = 0; i < penalty.size(); ++i) {
			penaltyArgs[i].SetNumber(penalty[i]);
		}
		penaltyArgs[3].SetBoolean(false);
		if (Perf::Invoke(_object, "updatePenalty", nullptr, penaltyArgs, 4) && _bCacheReady) {
			_penaltyCache.Store(penalty);
		}
		Perf::Count(Perf::Counter::PenaltySent);
	}

	void PlayerWidget::ToggleVanillaBars(bool a_bEnable)
	{
		auto hud = RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME);
		if (hud) {
			auto& uiMovie = hud->uiMovie;
			if (uiMovie) {
				RE::GFxValue bEnable;
				bEnable.SetBoolean(a_bEnable);
				/*uiMovie->SetVariable("HUDMovieBaseInstance.Health._visible", bEnable);
			uiMovie->SetVariable("HUDMovieBaseInstance.Magica._visible", bEnable);
			uiMovie->SetVariable("HUDMovieBaseInstance.Stamina._visible", bEnable);*/
				uiMovie->SetVariable("HUDMovieBaseInstance.Health.HealthMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Health.HealthPenaltyMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaFlashInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Magica.MagickaPenaltyMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaMeter_mc._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaFlashInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.Stamina.StaminaPenaltyMeter_mc._visible", bEnable);

				_bVanillaBarsHidden = !a_bEnable;
			}
		}
	}

	void PlayerWidget::ToggleVanillaEnchantmentChargeMeters(bool a_bEnable)
	{
		auto hud = RE::UI::GetSingleton()->GetMenu(RE::HUDMenu::MENU_NAME);
		if (hud) {
			auto& uiMovie = hud->uiMovie;
			if (uiMovie) {
				RE::GFxValue bEnable;
				bEnable.SetBoolean(a_bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.BottomLeftLockInstance._visible", bEnable);
				uiMovie->SetVariable("HUDMovieBaseInstance.BottomRightLockInstance._visible", bEnable);

				_bVanillaEnchantmentChargeMetersHidden = !a_bEnable;
			}
		}			
	}
}
