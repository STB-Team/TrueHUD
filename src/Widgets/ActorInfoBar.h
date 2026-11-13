#pragma once
#include "Widgets/InfoBarBase.h"

namespace Scaleform
{
	class ActorInfoBar : public InfoBarBase
	{
	public:
		ActorInfoBar(RE::GPtr<RE::GFxMovieView> a_view, RE::ObjectRefHandle a_refHandle) :
			InfoBarBase(a_view, a_refHandle)
		{
			_barType = InfoBarBase::kActorInfoBar;
		}

		virtual void Update(float a_deltaTime) override;

	protected:
		virtual void UpdatePosition() override;
		virtual void LoadConfig() override;

		// Last scale actually pushed to the movie clip. Scaleform caches tessellated
		// geometry and rendered filters per scale, so a scale that changes by a hair
		// every frame throws all of that away every frame.
		float _lastAppliedScale = -1.f;
		UpdateCache<3> _positionCache;
	};
}
