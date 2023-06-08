#pragma once

namespace loader::logic
{
	extern void Think();
	extern void OnStateChanged(States oldState, States newState);

	extern void Initialize();
	extern void Authorize();
	extern void Load();
}
