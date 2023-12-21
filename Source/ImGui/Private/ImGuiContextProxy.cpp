// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiContextProxy.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiImplementation.h"
#include "ImGuiInteroperability.h"
#include "Utilities/Arrays.h"
#include "VersionCompatibility.h"

#include <GenericPlatform/GenericPlatformFile.h>
#include <Misc/Paths.h>


static constexpr float DEFAULT_CANVAS_WIDTH = 3840.f;
static constexpr float DEFAULT_CANVAS_HEIGHT = 2160.f;


namespace
{
	FString GetSaveDirectory()
	{
#if ENGINE_COMPATIBILITY_LEGACY_SAVED_DIR
		const FString SavedDir = FPaths::GameSavedDir();
#else
		const FString SavedDir = FPaths::ProjectSavedDir();
#endif

		FString Directory = FPaths::Combine(*SavedDir, TEXT("ImGui"));

		// Make sure that directory is created.
		IPlatformFile::GetPlatformPhysical().CreateDirectory(*Directory);

		return Directory;
	}

	FString GetIniFile(const FString& Name)
	{
		static FString SaveDirectory = GetSaveDirectory();
		return FPaths::Combine(SaveDirectory, Name + TEXT(".ini"));
	}

	struct FGuardCurrentContext
	{
		FGuardCurrentContext()
			: OldContext(ImGui::GetCurrentContext())
		{
		}

		~FGuardCurrentContext()
		{
			if (bRestore)
			{
				ImGui::SetCurrentContext(OldContext);
			}
		}

		FGuardCurrentContext(FGuardCurrentContext&& Other)
			: OldContext(MoveTemp(Other.OldContext))
		{
			Other.bRestore = false;
		}

		FGuardCurrentContext& operator=(FGuardCurrentContext&&) = delete;

		FGuardCurrentContext(const FGuardCurrentContext&) = delete;
		FGuardCurrentContext& operator=(const FGuardCurrentContext&) = delete;

	private:

		ImGuiContext* OldContext = nullptr;
		bool bRestore = true;
	};
}

FImGuiContextProxy::FImGuiContextProxy(const FString& InName, int32 InContextIndex, ImFontAtlas* InFontAtlas, float InDPIScale)
	: Name(InName)
	, ContextIndex(InContextIndex)
	, IniFilename(TCHAR_TO_ANSI(*GetIniFile(InName)))
{
	// Create context.
	Context = ImGui::CreateContext(InFontAtlas);

	// Set this context in ImGui for initialization (any allocations will be tracked in this context).
	SetAsCurrent();

	// Start initialization.
	ImGuiIO& IO = ImGui::GetIO();

	ImGui::StyleColorsDark();

	{
		// Rounded Visual Studio style by RedNicStone from ImThemes
		ImGuiStyle& style = ImGui::GetStyle();

		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.6000000238418579f;
		style.WindowPadding = ImVec2(8.0f, 8.0f);
		style.WindowRounding = 4.0f;
		style.WindowBorderSize = 0.0f;
		style.WindowMinSize = ImVec2(32.0f, 32.0f);
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_Left;
		style.ChildRounding = 0.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupRounding = 4.0f;
		style.PopupBorderSize = 1.0f;
		style.FramePadding = ImVec2(4.0f, 3.0f);
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 0.0f;
		style.ItemSpacing = ImVec2(8.0f, 4.0f);
		style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
		style.CellPadding = ImVec2(4.0f, 2.0f);
		style.IndentSpacing = 21.0f;
		style.ColumnsMinSpacing = 6.0f;
		style.ScrollbarSize = 11.0f;
		style.ScrollbarRounding = 2.5f;
		style.GrabMinSize = 10.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 3.5f;
		style.TabBorderSize = 0.0f;
		style.TabMinWidthForCloseButton = 0.0f;
		style.ColorButtonPosition = ImGuiDir_Right;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

		style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f,
		                                             1.0f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f,
		                                             1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f,
		                                               1.0f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                              1.0f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                                 1.0f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarBg] =
			ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrab] =
			ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3529411852359772f, 0.3529411852359772f,
		                                                     0.3725490272045135f, 1.0f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3529411852359772f, 0.3529411852359772f,
		                                                    0.3725490272045135f, 1.0f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f,
		                                              1.0f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f,
		                                             1.0f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f,
		                                              1.0f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f,
		                                                 1.0f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f,
		                                                1.0f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f,
		                                                  1.0f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f,
		                                                 1.0f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                             1.0f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f,
		                                                 1.0f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f,
		                                                     0.9254902005195618f, 1.0f);
		style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f,
		                                              1.0f);
		style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f,
		                                                  1.0f);
		style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f,
		                                                 1.0f);
		style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                               1.0f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                             1.0f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f,
		                                                  0.2000000029802322f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f,
		                                                 1.0f);
	}

	// Set session data storage.
	IO.IniFilename = IniFilename.c_str();

	// Start with the default canvas size.
	ResetDisplaySize();
	IO.DisplaySize = {(float)DisplaySize.X, (float)DisplaySize.Y};

	// Set the initial DPI scale.
	SetDPIScale(InDPIScale);

	// Initialize key mapping, so context can correctly interpret input state.
	ImGuiInterops::SetUnrealKeyMap(IO);

	// Begin frame to complete context initialization (this is to avoid problems with other systems calling to ImGui
	// during startup).
	BeginFrame();
}

FImGuiContextProxy::~FImGuiContextProxy()
{
	if (Context)
	{
		// It seems that to properly shutdown context we need to set it as the current one (at least in this framework
		// version), even though we can pass it to the destroy function.
		SetAsCurrent();

		// Save context data and destroy.
		ImGui::DestroyContext(Context);
	}
}

void FImGuiContextProxy::ResetDisplaySize()
{
	DisplaySize = { DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT };
}

void FImGuiContextProxy::SetDPIScale(float Scale)
{
	if (DPIScale != Scale)
	{
		DPIScale = Scale;

		ImGuiStyle NewStyle = ImGuiStyle();
		NewStyle.ScaleAllSizes(Scale);

		FGuardCurrentContext GuardContext;
		SetAsCurrent();
		ImGui::GetStyle() = MoveTemp(NewStyle);
	}
}

void FImGuiContextProxy::DrawEarlyDebug()
{
	if (bIsFrameStarted && !bIsDrawEarlyDebugCalled)
	{
		bIsDrawEarlyDebugCalled = true;

		SetAsCurrent();

		// Delegates called in order specified in FImGuiDelegates.
		BroadcastMultiContextEarlyDebug();
		BroadcastWorldEarlyDebug();
	}
}

void FImGuiContextProxy::DrawDebug()
{
	if (bIsFrameStarted && !bIsDrawDebugCalled)
	{
		bIsDrawDebugCalled = true;

		// Make sure that early debug is always called first to guarantee order specified in FImGuiDelegates.
		DrawEarlyDebug();

		SetAsCurrent();

		// Delegates called in order specified in FImGuiDelegates.
		BroadcastWorldDebug();
		BroadcastMultiContextDebug();
	}
}

void FImGuiContextProxy::Tick(float DeltaSeconds)
{
	// Making sure that we tick only once per frame.
	if (LastFrameNumber < GFrameNumber)
	{
		LastFrameNumber = GFrameNumber;

		SetAsCurrent();

		if (bIsFrameStarted)
		{
			// Make sure that draw events are called before the end of the frame.
			DrawDebug();

			// Ending frame will produce render output that we capture and store for later use. This also puts context to
			// state in which it does not allow to draw controls, so we want to immediately start a new frame.
			EndFrame();
		}

		// Update context information (some data need to be collected before starting a new frame while some other data
		// may need to be collected after).
		bHasActiveItem = ImGui::IsAnyItemActive();
		MouseCursor = ImGuiInterops::ToSlateMouseCursor(ImGui::GetMouseCursor());

		// Begin a new frame and set the context back to a state in which it allows to draw controls.
		BeginFrame(DeltaSeconds);

		// Update remaining context information.
		bWantsMouseCapture = ImGui::GetIO().WantCaptureMouse;
	}
}

void FImGuiContextProxy::BeginFrame(float DeltaTime)
{
	if (!bIsFrameStarted)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DeltaTime = DeltaTime;

		ImGuiInterops::CopyInput(IO, InputState);
		InputState.ClearUpdateState();

		IO.DisplaySize = { (float)DisplaySize.X, (float)DisplaySize.Y };

		ImGui::NewFrame();

		bIsFrameStarted = true;
		bIsDrawEarlyDebugCalled = false;
		bIsDrawDebugCalled = false;
	}
}

void FImGuiContextProxy::EndFrame()
{
	if (bIsFrameStarted)
	{
		// Prepare draw data (after this call we cannot draw to this context until we start a new frame).
		ImGui::Render();

		// Update our draw data, so we can use them later during Slate rendering while ImGui is in the middle of the
		// next frame.
		UpdateDrawData(ImGui::GetDrawData());

		bIsFrameStarted = false;
	}
}

void FImGuiContextProxy::UpdateDrawData(ImDrawData* DrawData)
{
	if (DrawData && DrawData->CmdListsCount > 0)
	{
		DrawLists.SetNum(DrawData->CmdListsCount, false);

		for (int Index = 0; Index < DrawData->CmdListsCount; Index++)
		{
			DrawLists[Index].TransferDrawData(*DrawData->CmdLists[Index]);
		}
	}
	else
	{
		// If we are not rendering then this might be a good moment to empty the array.
		DrawLists.Empty();
	}
}

void FImGuiContextProxy::BroadcastWorldEarlyDebug()
{
	if (ContextIndex != Utilities::INVALID_CONTEXT_INDEX)
	{
		FSimpleMulticastDelegate& WorldEarlyDebugEvent = FImGuiDelegatesContainer::Get().OnWorldEarlyDebug(ContextIndex);
		if (WorldEarlyDebugEvent.IsBound())
		{
			WorldEarlyDebugEvent.Broadcast();
		}
	}
}

void FImGuiContextProxy::BroadcastMultiContextEarlyDebug()
{
	FSimpleMulticastDelegate& MultiContextEarlyDebugEvent = FImGuiDelegatesContainer::Get().OnMultiContextEarlyDebug();
	if (MultiContextEarlyDebugEvent.IsBound())
	{
		MultiContextEarlyDebugEvent.Broadcast();
	}
}

void FImGuiContextProxy::BroadcastWorldDebug()
{
	if (DrawEvent.IsBound())
	{
		DrawEvent.Broadcast();
	}

	if (ContextIndex != Utilities::INVALID_CONTEXT_INDEX)
	{
		FSimpleMulticastDelegate& WorldDebugEvent = FImGuiDelegatesContainer::Get().OnWorldDebug(ContextIndex);
		if (WorldDebugEvent.IsBound())
		{
			WorldDebugEvent.Broadcast();
		}
	}
}

void FImGuiContextProxy::BroadcastMultiContextDebug()
{
	FSimpleMulticastDelegate& MultiContextDebugEvent = FImGuiDelegatesContainer::Get().OnMultiContextDebug();
	if (MultiContextDebugEvent.IsBound())
	{
		MultiContextDebugEvent.Broadcast();
	}
}
