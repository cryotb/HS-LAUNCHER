#pragma once

namespace loader::ui
{
	//
	// Global
	//
	inline UI::C_Slider status_slider{};

	//
	// Drawing
	//
	extern void DrawBase();
	extern void DrawError();
	extern void DrawInitializing();
	extern void DrawAuthorizing();
	extern void DrawSelection();
	extern void DrawLoadingPrompt();
	extern void DrawLoading();

	//
	// Main
	//
	extern bool Destroy();
	extern void ExitSafe();
	extern void Render();
}
