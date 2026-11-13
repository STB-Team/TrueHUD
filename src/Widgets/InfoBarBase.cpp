#include "Widgets/InfoBarBase.h"
#include "Offsets.h"
#include "Settings.h"
#include "Utils.h"
#include "HUDHandler.h"
#include "NPCNameProvider.h"
#include "Perf.h"

namespace Scaleform
{
	void InfoBarBase::Initialize()
	{
		_flashInitialized = false;
		_dataCache.Reset();
		_specialCache.Reset();
		LoadConfig();
		SetWidgetState(WidgetStateMode::kShow);
	}

	void InfoBarBase::Dispose()
	{
		_object.Invoke("cleanUp", nullptr, nullptr, 0);
	}

	void InfoBarBase::SetWidgetState(WidgetStateMode a_state)
	{
		switch (a_state) {
		case WidgetStateMode::kAdd:
			if (_widgetState == kPendingHide || _widgetState == kPendingRemoval || _widgetState == kHidden) {
				_widgetState = kActive;
			}
		case WidgetStateMode::kShow:
			if (_widgetState == kPendingHide || _widgetState == kHidden) {
				_widgetState = kActive;
			}
			break;
		case WidgetStateMode::kHide:
			if (_widgetState == kActive) {
				_widgetState = kPendingHide;
			}
			break;
		case WidgetStateMode::kRemove:
			if (_widgetState < kPendingRemoval) {
				_widgetState = kPendingRemoval;
			}
			break;
		case WidgetStateMode::kTargetKilled:
			if (_widgetState < kDelayedRemoval) {
				_widgetState = kDelayedRemoval;
			}
			break;
		}
	}

	void InfoBarBase::FlashActorValue(RE::ActorValue a_actorValue, bool a_bLong)
	{
		RE::GFxValue args[2];
		args[0].SetNumber(static_cast<int32_t>(a_actorValue));
		args[1].SetBoolean(a_bLong);

		_object.Invoke("flashAV", nullptr, args, 2);
	}

	void InfoBarBase::FlashSpecial(bool a_bLong)
	{
		RE::GFxValue arg;
		arg.SetBoolean(a_bLong);

		_object.Invoke("flashSpecial", nullptr, &arg, 1);
	}

	void InfoBarBase::RefreshColors()
	{
		using ColorType = TRUEHUD_API::BarColorType;

		auto hudMenu = HUDHandler::GetTrueHUDMenu();

		RE::GFxValue args[20];
		args[0].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kHealth, ColorType::BarColor));
		args[1].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kHealth, ColorType::PhantomColor));
		args[2].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kHealth, ColorType::BackgroundColor));
		args[3].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kHealth, ColorType::PenaltyColor));
		args[4].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kHealth, ColorType::FlashColor));
		args[5].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kMagicka, ColorType::BarColor));
		args[6].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kMagicka, ColorType::PhantomColor));
		args[7].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kMagicka, ColorType::BackgroundColor));
		args[8].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kMagicka, ColorType::PenaltyColor));
		args[9].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kMagicka, ColorType::FlashColor));
		args[10].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kStamina, ColorType::BarColor));
		args[11].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kStamina, ColorType::PhantomColor));
		args[12].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kStamina, ColorType::BackgroundColor));
		args[13].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kStamina, ColorType::PenaltyColor));
		args[14].SetNumber(hudMenu->GetBarColor(_refHandle, RE::ActorValue::kStamina, ColorType::FlashColor));
		args[15].SetNumber(hudMenu->GetSpecialBarColor(_refHandle, ColorType::BarColor));
		args[16].SetNumber(hudMenu->GetSpecialBarColor(_refHandle, ColorType::PhantomColor));
		args[17].SetNumber(hudMenu->GetSpecialBarColor(_refHandle, ColorType::BackgroundColor));
		args[18].SetNumber(hudMenu->GetSpecialBarColor(_refHandle, ColorType::PenaltyColor));
		args[19].SetNumber(hudMenu->GetSpecialBarColor(_refHandle, ColorType::FlashColor));
		_object.Invoke("setColors", nullptr, args, 20);
	}
	namespace
	{
		float actor_value_regeneration_value(RE::ActorValue av, RE::Actor* actor)
		{
			if (!actor) {
				return 0.f;
			}

			const auto active_effects = actor->AsMagicTarget()->GetActiveEffectList();
			if (!active_effects) {
				return 0.f;
			}
			auto num = 0.f;
			for (auto active_effect : *active_effects) {
				if (active_effect->effect->baseEffect->data.primaryAV == av &&
					!active_effect->flags.any(RE::ActiveEffect::Flag::kInactive) &&
					!active_effect->effect->baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kRecover) &&
					!active_effect->effect->baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
					num += active_effect->magnitude;
				}
			}

			return num;
		}

		RE::BGSPerk* PoisonPerk = nullptr;
		RE::BGSKeyword* PoisonKw = nullptr;
		RE::SpellItem* BossStaggerKD = nullptr;
		bool bLookedUpForms = false;

		// A failed lookup used to leave PoisonPerk null, which retried all three lookups
		// every frame on every widget - and LookupByEditorID takes a lock.
		void LookupForms()
		{
			if (bLookedUpForms) {
				return;
			}
			bLookedUpForms = true;

			if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
				PoisonPerk = dataHandler->LookupForm<RE::BGSPerk>(0x01928D, "STB.esp");
			}
			PoisonKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("aaMZkw_PoisonDamage");
			BossStaggerKD = RE::TESForm::LookupByEditorID<RE::SpellItem>("aaMZs_Boss_AllStaggerKD");
		}
	}

	void InfoBarBase::RefreshCachedInfo(RE::Actor* a_actor, RE::PlayerCharacter* a_playerCharacter, bool a_bIsTarget)
	{
		Perf::Scope scope(Perf::Phase::CachedInfo);
		LookupForms();

		_cachedIsHostile = a_actor->IsHostileToActor(a_playerCharacter);
		_cachedIsTeammate = Utils::IsPlayerTeammateOrSummon(a_actor);
		_cachedName = NPCNameProvider::GetSingleton()->GetName(a_actor);
		_cachedLevel = a_actor->GetLevel();

		if (Settings::kywd_Dragon && a_actor->HasKeyword(Settings::kywd_Dragon)) {
			_cachedSoulType = 7;
		} else if (Settings::kywd_noSoulTrap && a_actor->HasKeyword(Settings::kywd_noSoulTrap)) {
			_cachedSoulType = 0;
		} else if (auto isSentient = IsSentient(a_actor)) {  // If sentient, then don't bother - all sentients have black soul gem levels
			_cachedSoulType = 6;
		} else {
			_cachedSoulType = GetSoulType(_cachedLevel, static_cast<uint8_t>(isSentient));
		}

		// The line of sight check is a ray cast - by far the most expensive thing in here.
		if (a_bIsTarget) {
			_cachedHasLineOfSight = true;
		} else {
			Perf::Scope losScope(Perf::Phase::LineOfSight);
			bool r8 = false;
			_cachedHasLineOfSight = a_playerCharacter->HasLineOfSight(a_actor, r8);
		}

		float health_regen = actor_value_regeneration_value(RE::ActorValue::kHealth, a_actor);
		if (health_regen > 0.f && _cachedIsHostile && PoisonPerk && PoisonKw && a_playerCharacter->HasPerk(PoisonPerk) &&
			a_actor->AsMagicTarget()->HasMagicEffectWithKeyword(PoisonKw, 0)) {
			health_regen *= 0.5f;
		}
		_cachedRegenString = fmt::format("{:.{}f}"sv, health_regen, 2);

		_cachedIsBoss = HUDHandler::GetSingleton()->CheckActorForBoss(a_actor->GetHandle());
	}

	void InfoBarBase::UpdateInfo(float a_deltaTime)
	{
		Perf::Scope scope(Perf::Phase::Info);
		auto playerCharacter = RE::PlayerCharacter::GetSingleton();
		auto playerLevel = playerCharacter->GetLevel();

		const auto reference = _refHandle.get();
		if (!reference) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		auto actor = reference->As<RE::Actor>();
		if (!actor) {
			_widgetState = WidgetState::kRemoved;
			return;
		}

		const bool canCache = _flashInitialized;
		if (!_flashInitialized) {
			Perf::Scope readinessScope(Perf::Phase::Invoke);
			RE::GFxValue initialized;
			if (_object.GetMember("bInitialized", &initialized) && initialized.IsBool()) {
				_flashInitialized = initialized.GetBool();
			}
		}

		auto hudMenu = HUDHandler::GetTrueHUDMenu();

		auto targetHandle = hudMenu->GetTarget();
		auto softTargetHandle = hudMenu->GetSoftTarget();

		// Whether this actor is the player's target has to be re-evaluated every frame,
		// everything else behind the target type is cached.
		bool bIsTarget = (targetHandle && _refHandle == targetHandle) || (!targetHandle && _refHandle == softTargetHandle);

		_cachedInfoTimer -= a_deltaTime;
		if (_cachedInfoTimer < 0.f) {
			RefreshCachedInfo(actor, playerCharacter, bIsTarget);
			// Spread the refreshes out over the interval so every visible bar doesn't
			// land on the same frame.
			_cachedInfoTimer = Settings::fWidgetSlowUpdateInterval * (1.f + static_cast<float>(_widgetID % 16) * 0.03125f);
		}

		bool bIsTeammate = _cachedIsTeammate;

		TargetType targetType;
		if (bIsTarget) {
			targetType = kTarget;
		} else if (_cachedIsHostile) {
			targetType = kEnemy;
		} else if (bIsTeammate) {
			targetType = kTeammate;
		} else {
			targetType = kOther;
		}

		if (targetType == kTarget && _widgetState == kPendingRemoval) {
			SetWidgetState(WidgetStateMode::kAdd);
		}

		TargetState targetState;
		if (actor->IsDead()) {
			targetState = kDead;
		} else if (actor->AsActorState()->IsBleedingOut()) {
			targetState = actor->IsEssential() ? kBleedingOutEssential : kBleedingOut;
		} else {
			targetState = kAlive;
		}

		if (_widgetState < kDelayedRemoval) {
			// bHidden can only flip to true while the hide animation is running, so there's
			// no point paying for the round trip into the VM in any other state.
			if (_widgetState == kPendingHide) {
				RE::GFxValue result;
				_object.Invoke("isHidden", &result);
				if (result.GetBool()) {
					_widgetState = WidgetState::kHidden;
				}
			}

			const auto shouldBarBeDisplayed = [&]() {
				if (_barType == BarType::kBossInfoBar) {
					return !bIsTeammate && actor->IsInCombat() && _cachedIsHostile;
				} else if (_barType == BarType::kActorInfoBar) {
					if (targetType > kTarget && !actor->IsInCombat()) {
						return false;
					}
					switch (targetType) {
					case TargetType::kTarget:
						return true;
					case TargetType::kEnemy:
						return Settings::uInfoBarDisplayHostiles > InfoBarsDisplayMode::kNever;
					case TargetType::kTeammate:
						return Settings::uInfoBarDisplayTeammates > InfoBarsDisplayMode::kNever;
					case TargetType::kOther:
						return Settings::uInfoBarDisplayOthers > InfoBarsDisplayMode::kNever;
					}
				}
				return false;
			};

			if (targetState > kBleedingOut) {
				SetWidgetState(WidgetStateMode::kTargetKilled);
			} else if (!shouldBarBeDisplayed()) {
				SetWidgetState(WidgetStateMode::kRemove);
			} else if (_barType == BarType::kActorInfoBar) {
				if (_bIsOffscreen) {
					SetWidgetState(WidgetStateMode::kHide);
				} else {
					bool bHasLOS = targetType == kTarget ? true : _cachedHasLineOfSight;
					bool bVisible = bHasLOS && !(actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kInvisibility) > 0);
					if (bVisible) {
						SetWidgetState(WidgetStateMode::kShow);
					} else {
						SetWidgetState(WidgetStateMode::kHide);
					}
				}
			} 
		}

		const auto& targetName = _cachedName;
		auto targetLevel = _cachedLevel;

		uint32_t levelColor = Settings::uDefaultColor;
		uint32_t outlineColor = Settings::uDefaultColorOutline;
		if (bIsTeammate) {
			levelColor = Settings::uTeammateColor;
			outlineColor = Settings::uTeammateColorOutline;
		} else if (playerLevel - targetLevel > Settings::uInfoBarLevelThreshold) {
			levelColor = Settings::uWeakerColor;
			if (Settings::bInfoBarLevelColorOutline) {
				outlineColor = Settings::uWeakerColorOutline;
			}
		} else if (targetLevel - playerLevel > Settings::uInfoBarLevelThreshold) {
			levelColor = Settings::uStrongerColor;
			if (Settings::bInfoBarLevelColorOutline) {
				outlineColor = Settings::uStrongerColorOutline;
			}
		}

		uint32_t soulType = _cachedSoulType;

		auto actorValueOwner = actor->AsActorValueOwner();
		float permanentHealth = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kHealth);
		float temporaryHealth = actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kHealth);
		float maxHealth = permanentHealth + temporaryHealth;
		float health;
		if (targetState == kDead || targetState == kBleedingOutEssential) {
			health = 0;
		} else {
			health = actorValueOwner->GetActorValue(RE::ActorValue::kHealth);
		}
		float permanentMagicka = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kMagicka);
		float temporaryMagicka = actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kMagicka);
		float maxMagicka = permanentMagicka + temporaryMagicka;
		float magicka = actorValueOwner->GetActorValue(RE::ActorValue::kMagicka);
		float permanentStamina = actorValueOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
		float temporaryStamina = actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);
		float maxStamina = permanentStamina + temporaryStamina;
		float stamina = actorValueOwner->GetActorValue(RE::ActorValue::kStamina);
		const UpdateCache<17, 2>::Numbers data{
			static_cast<double>(targetType), static_cast<double>(_widgetState), _depth,
			static_cast<double>(targetState), static_cast<double>(targetLevel),
			static_cast<double>(levelColor), static_cast<double>(outlineColor), static_cast<double>(soulType),
			ceil(maxHealth), ceil(health), Utils::GetPct(health, maxHealth),
			ceil(maxMagicka), ceil(magicka), Utils::GetPct(magicka, maxMagicka),
			ceil(maxStamina), ceil(stamina), Utils::GetPct(stamina, maxStamina)
		};
		const UpdateCache<17, 2>::Strings strings{ targetName, _cachedRegenString };
		const bool sendData = !Perf::UseOptimizations() || !canCache || !_dataCache.Matches(data, strings);
		if (sendData) {
			RE::GFxValue args[19];
			for (std::size_t i = 0; i < 4; ++i) {
				args[i].SetNumber(data[i]);
			}
			args[4].SetString(targetName);
			for (std::size_t i = 4; i < data.size(); ++i) {
				args[i + 1].SetNumber(data[i]);
			}
			args[18].SetString(_cachedRegenString);
			if (Perf::Invoke(_object, "updateData", nullptr, args, 19) && canCache) {
				_dataCache.Store(data, strings);
			}
		}
		Perf::Count(sendData ? Perf::Counter::DataSent : Perf::Counter::DataSkipped);
		Perf::Bar(_widgetID, _widgetState, targetType, health, maxHealth, sendData);

		if (_cachedIsBoss) {
			float maxSpecial = 0;
			float currentSpecial = 0;
			if (actor->AsMagicTarget() && actor->AsMagicTarget()->GetActiveEffectList())
			for (auto ef : *actor->AsMagicTarget()->GetActiveEffectList())
				if (ef->spell == BossStaggerKD) {
					maxSpecial = fmax(ef->duration, 0.f);
					currentSpecial = fmax(ef->duration - ef->elapsedSeconds, 0.f);
				}

			const UpdateCache<3>::Numbers special{ ceil(maxSpecial), ceil(currentSpecial), Utils::GetPct(currentSpecial, maxSpecial) };
			const bool sendSpecial = !Perf::UseOptimizations() || !canCache || !_specialCache.Matches(special);
			if (sendSpecial) {
				RE::GFxValue specialArgs[3];
				for (std::size_t i = 0; i < special.size(); ++i) {
					specialArgs[i].SetNumber(special[i]);
				}
				if (Perf::Invoke(_object, "updateSpecial", nullptr, specialArgs, 3) && canCache) {
					_specialCache.Store(special);
				}
			}
			Perf::Count(sendSpecial ? Perf::Counter::SpecialSent : Perf::Counter::SpecialSkipped);
		}
	//	const RE::GFxValue var5{ ceil(health) };
	//	RE::GFxValue var;
	//	//_view->GetVariable(&var, "_root.TrueHUD.Bars.HealthBar.BurNumValue");
	//	//var.SetMember("htmlText", var5);
	//	//var.SetMember("text", var5);
	//	//_object.GetMember("Bars.HealthBar.BurNumValue.htmlText", &var);
	//	//var.SetNumber(ceil(health));
	//	//_object.GetMember("Bars.HealthBar.BurNumValue.text", &var);
	//	//var.SetNumber(ceil(health));
	//	_object.GetMember("Bars.HealthBar.BurNumValue", &var);
	//	var.SetMember("htmlText", var5);
	//	//_object.GetMember("Bars.HealthBar.BurNumValue", &var);
	//	//var.SetMember("text", var5);
		
	}
}
