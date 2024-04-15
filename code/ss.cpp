#define SLIDE_WIDTH  640
#define SLIDE_HEIGHT 360

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>

#include "dear_imgui/imgui.h"
#include "dear_imgui/imgui_internal.h"
#include "dear_imgui/imgui_impl_sdl.h"
#include "dear_imgui/imgui_impl_opengl3.h"

#define SDL_MAIN_HANDLED
#include "sdl/SDL.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include "sdl/SDL_opengles2.h"
#else
#include "sdl/SDL_opengl.h"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum ss_placement_mode
{
    PLACEMENT_MODE_NONE,
    PLACEMENT_MODE_TEXT,
    PLACEMENT_MODE_IMAGE
};

#define SS_TEXT_LENGTH 2046
struct ss_placed_text
{
    WCHAR Text[SS_TEXT_LENGTH];
    ImU32 Color;
    ImVec2 Position;
    float Size;
    bool Selected;
};

struct ss_placed_image
{
    WCHAR FilePath[MAX_PATH];
    GLuint Tex;
    ImVec2 Position;
    ImVec2 Size;
    bool SizeRatioLocked;
    float SizeRatio;
    bool Selected;
};

#define SS_MAX_TEXTS  100
#define SS_MAX_IMAGES 50
struct ss_slide
{
    int TextCount;
    ss_placed_text Texts[SS_MAX_TEXTS];
    int ImageCount;
    ss_placed_image Images[SS_MAX_IMAGES];
    ImU32 BGColor;
};

// NOTE(evan): SaveFileName MUST be the first element
struct ss_state
{
    WCHAR SaveFileName[MAX_PATH];
    WCHAR ConfigFileName[MAX_PATH];
    WCHAR EXEPath[MAX_PATH];
    
    char ProjectName[SS_TEXT_LENGTH];
    
    WCHAR PlaceholderImageName[MAX_PATH];
    GLuint PlaceholderImageTexture;
    ImVec2 PlaceholderImageSize;
    
    HINSTANCE Instance;
    ss_placement_mode PlacementMode;
    int SlideCount;
    int SlideIndex;
    ss_slide *Slides;
};

#define ALLOC_STRUCT(Count, PPStruct, type) \
*PPStruct = (type *)SDL_malloc(Count * sizeof(type)); \
memset(*PPStruct, 0, Count * sizeof(type));

static void AddText(ss_slide *Slide);
static bool LoadTexture(char *FileName, GLuint *Texture, int *Width, int *Height);
#include "ss_ini.cpp"
#include "ss_commands.cpp"

static bool ProcessKey(ss_state *State, SDL_Keysym Sym, bool Pressed, bool *SettingProjectTitle);

int main(int, char **)
{
    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER))
    {
        printf("Error: %s\n", SDL_GetError());
        return(-1);
    }
    
#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char *GLSLVersion = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char *GLSLVersion = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags WindowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL|
                                                    SDL_WINDOW_RESIZABLE|
                                                    SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *Window = 
        SDL_CreateWindow("Simply Slide",
                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1024, 576, WindowFlags);
    SDL_GLContext GLContext = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, GLContext);
    SDL_GL_SetSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &IO = ImGui::GetIO();
    IO.ConfigFlags |= (ImGuiConfigFlags_NavEnableKeyboard|
                       ImGuiConfigFlags_DockingEnable|
                       ImGuiConfigFlags_ViewportsEnable);
    ImFontConfig FontConfig;
    FontConfig.OversampleH = 3;
    FontConfig.OversampleV = 3;
    FontConfig.GlyphExtraSpacing.x = 1.0f;
    IO.Fonts->AddFontFromFileTTF("Roboto.ttf", 14.0f, &FontConfig);
    ImFont *RegTextFont = IO.Fonts->AddFontFromFileTTF("Roboto.ttf", 20.0f, &FontConfig);
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL2_InitForOpenGL(Window, GLContext);
    ImGui_ImplOpenGL3_Init(GLSLVersion);
    ImVec4 ClearColor = ImVec4(0.02314f, 0.02314f, 0.02314f, 1.0f);
    
    ss_state State = { L"\0" };
    State.Instance = GetModuleHandleA(0);
    GetEXEPath(State.EXEPath);
    LoadAppFromConfig(&State, L"config.ss");
    if(wcscmp(State.SaveFileName, L"\0"))
    {
        GetProjectFromIni(&State);
    }
    if(strcmp(State.ProjectName, "\0"))
    {
        char Title[SS_TEXT_LENGTH];
        sprintf(Title, "Simply Slide Project: %s", State.ProjectName);
        SDL_SetWindowTitle(Window, Title);
    }
    
    bool SlidesWindowOpened = true;
    bool ViewportWindowOpened = true;
    
    int WindowWidth = 0, WindowHeight = 0;
    
    int MouseX = 0, MouseY = 0;
    bool MouseLDown = false;
    bool MouseRDown = false;
    
    bool ProjectOpen = false;
    
    bool SettingProjectTitle = false;
    bool Running = true;
    while(Running)
    {
        ProjectOpen = false;
        if(wcscmp(State.SaveFileName, L"\0"))
        {
            ProjectOpen = true;
        }
        
        SDL_Event Event;
        while(SDL_PollEvent(&Event))
        {
            ImGui_ImplSDL2_ProcessEvent(&Event);
            if(Event.type == SDL_QUIT)
            {
                Running = false;
            }
            if(Event.type == SDL_WINDOWEVENT &&
               Event.window.event == SDL_WINDOWEVENT_CLOSE &&
               Event.window.windowID == SDL_GetWindowID(Window))
            {
                Running = false;
            }
            
            if(Event.type == SDL_KEYDOWN)
            {
                if(ProcessKey(&State, Event.key.keysym, true, &SettingProjectTitle))
                {
                    Running = false;
                }
            }
            if(Event.type == SDL_KEYUP)
            {
                if(ProcessKey(&State, Event.key.keysym, false, &SettingProjectTitle))
                {
                    Running = false;
                }
            }
        }
        {
            SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);
            Uint32 Buttons = SDL_GetMouseState(&MouseX, &MouseY);
            MouseLDown = (Buttons & SDL_BUTTON_LMASK) != 0;
            MouseRDown = (Buttons & SDL_BUTTON_RMASK) != 0;
        }
        
        if(ProjectOpen && State.SlideCount == 0)
        {
            NewSlide(&State);
            State.SlideIndex = 0;
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::DockSpaceOverViewport();
        ImVec2 MenuBarSize;
        if(ImGui::BeginMainMenuBar())
        {
            MenuBarSize = ImGui::GetWindowSize();
            if(ImGui::BeginMenu("File"))
            {
                if(ImGui::MenuItem("New Project", "Ctrl+N"))
                {
                    SettingProjectTitle = true;
                    NewProject(&State, State.SaveFileName);
                }
                
                if(ImGui::MenuItem("Open", "Ctrl+O"))
                {
                    if(!OpenProject(&State, State.SaveFileName))
                        State.SaveFileName[0] = L'\0';
                }
                
                if(ProjectOpen && ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    Save(&State, State.SaveFileName);
                }
                
                ImGui::EndMenu();
            }
            
            if(ProjectOpen && ImGui::BeginMenu("Menu"))
            {
                if(ImGui::MenuItem("Set Project Title"))
                {
                    SettingProjectTitle = true;
                }
                
                if(ImGui::MenuItem("Compile", "Ctrl+B"))
                {
                    Compile(&State);
                }
                
                if(ImGui::MenuItem("Add Text"))
                {
                    State.PlacementMode = PLACEMENT_MODE_TEXT;
                }
                
                if(ImGui::MenuItem("Add Image"))
                {
                    State.PlacementMode = PLACEMENT_MODE_IMAGE;
                }
                ImGui::EndMenu();
            }
            
            if(ImGui::BeginMenu("Windows"))
            {
                if(ImGui::MenuItem("Slides"))
                    SlidesWindowOpened = !SlidesWindowOpened;
                if(ImGui::MenuItem("Viewport"))
                    ViewportWindowOpened = !ViewportWindowOpened;
                
                ImGui::EndMenu();
            }
            
            if(ProjectOpen && ImGui::BeginMenu("Slides"))
            {
                if(ImGui::MenuItem("New Slide", "Ctrl+Shift+N"))
                {
                    NewSlide(&State);
                }
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        if(SettingProjectTitle)
        {
            ImGui::OpenPopup("Set Project Title");
            if(ImGui::BeginPopupModal("Set Project Title"))
            {
                ImGui::InputText("##set_project_title", State.ProjectName, SS_TEXT_LENGTH);
                if(ImGui::Button("Ok"))
                {
                    // Set window title
                    char Title[SS_TEXT_LENGTH];
                    sprintf(Title, "Simply Slide Project: %s", State.ProjectName);
                    SDL_SetWindowTitle(Window, Title);
                    SettingProjectTitle = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
        if(SlidesWindowOpened)
        {
            ImGui::Begin("Slides", &SlidesWindowOpened);
            {
                for(int SlideIndex = 0;
                    SlideIndex < State.SlideCount;
                    ++SlideIndex)
                {
                    char NameBuffer[1024];
                    sprintf(NameBuffer, "Slide %d", SlideIndex);
                    if(ImGui::Button(NameBuffer))
                    {
                        ss_slide *PreviousSlide = &State.Slides[State.SlideIndex];
                        UnselectAll(PreviousSlide->TextCount, PreviousSlide->Texts,
                                    PreviousSlide->ImageCount, PreviousSlide->Images);
                        State.SlideIndex = SlideIndex;
                    }
                    if(ImGui::BeginPopupContextItem())
                    {
                        ImGui::Text(NameBuffer);
                        if(State.SlideCount > 1)
                        {
                            if(ImGui::Button("Delete"))
                            {
                                DeleteSlide(&State, SlideIndex);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        else
                        {
                            ImGui::Text("Cannot delete because it is the last slide");
                        }
                        if(ImGui::Button("Close"))
                            ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::End();
        }
        
        if(ViewportWindowOpened)
        {
            ImGui::Begin("Viewport", &ViewportWindowOpened);
            if(ProjectOpen && State.SlideIndex >= 0)
            {
                bool Clicked = false;
                if(ImGui::InvisibleButton("##viewport",
                                          ImVec2(SLIDE_WIDTH, SLIDE_HEIGHT)))
                {
                    Clicked = true;
                }
                
                ImDrawList *DrawList = ImGui::GetWindowDrawList();
                ImVec2 ViewportSize = ImGui::GetWindowSize();
                ImVec2 ViewportPos = ImGui::GetWindowPos();
                ImVec2 P0 = ImGui::GetItemRectMin();
                ImVec2 P1 = ImGui::GetItemRectMax();
                ImVec2 RelativeMouse = ImVec2(MouseX - P0.x, MouseY - P0.y);
                bool MouseOverSomething = false;
                {
                    ss_slide *Slide = &State.Slides[State.SlideIndex];
                    bool ShouldOpenPopup = false;
                    
                    ImGui::PushClipRect(P0, P1, true);
                    {
                        DrawList->AddRectFilled(P0, P1, Slide->BGColor);
                        
                        if(Clicked && State.PlacementMode == PLACEMENT_MODE_TEXT)
                        {
                            ImGui::PushFont(RegTextFont);
                            
                            AddText(Slide, L"Placeholder", RelativeMouse);
                            UnselectAll(Slide->TextCount, Slide->Texts, Slide->ImageCount, Slide->Images);
                            Slide->Texts[Slide->TextCount - 1].Selected = true;
                            State.PlacementMode = PLACEMENT_MODE_NONE;
                            
                            ImGui::PopFont();
                        }
                        
                        if(Clicked && State.PlacementMode == PLACEMENT_MODE_IMAGE)
                        {
                            AddImage(Slide, State.PlaceholderImageTexture, State.PlaceholderImageName,
                                     RelativeMouse, State.PlaceholderImageSize);
                            UnselectAll(Slide->TextCount, Slide->Texts, Slide->ImageCount, Slide->Images);
                            Slide->Images[Slide->ImageCount - 1].Selected = true;
                            State.PlacementMode = PLACEMENT_MODE_NONE;
                        }
                        
                        for(int TextIndex = 0;
                            TextIndex < Slide->TextCount;
                            ++TextIndex)
                        {
                            ss_placed_text *Text = &Slide->Texts[TextIndex];
                            char RegText[SS_TEXT_LENGTH];
                            wcstombs(RegText, Text->Text, SS_TEXT_LENGTH);
                            ImVec2 TextSize = RegTextFont->CalcTextSizeA(Text->Size, FLT_MAX, 0.0f, RegText);
                            ImVec2 TextPosition = ImVec2(Text->Position.x + P0.x, Text->Position.y + P0.y);
                            
                            DrawList->AddText(RegTextFont, Text->Size, TextPosition, Text->Color, RegText);
                            
                            if((float)MouseX > TextPosition.x && (float)MouseX < TextPosition.x + TextSize.x &&
                               (float)MouseY > TextPosition.y && (float)MouseY < TextPosition.y + TextSize.y &&
                               
                               (float)MouseX > P0.x && (float)MouseX < P1.x &&
                               (float)MouseY > P0.y && (float)MouseY < P1.y &&
                               
                               !SettingProjectTitle)
                            {
                                MouseOverSomething = true;
                                if(MouseLDown)
                                {
                                    UnselectAll(Slide->TextCount, Slide->Texts, Slide->ImageCount, Slide->Images);
                                    Text->Selected = true;
                                }
                                else if(!Text->Selected)
                                {
                                    DrawList->AddRect(TextPosition,
                                                      ImVec2(TextPosition.x + TextSize.x, TextPosition.y + TextSize.y),
                                                      IM_COL32(0x69, 0x69, 0x69, 120), 0, 0, 1);
                                }
                            }
                            
                            if(Text->Selected)
                            {
                                DrawList->AddRect(TextPosition,
                                                  ImVec2(TextPosition.x + TextSize.x, TextPosition.y + TextSize.y),
                                                  IM_COL32(0, 0, 0, 255), 0, 0, 3);
                            }
                        }
                        
                        for(int ImageIndex = 0;
                            ImageIndex < Slide->ImageCount;
                            ++ImageIndex)
                        {
                            ss_placed_image *Image = &Slide->Images[ImageIndex];
                            
                            ImVec2 Min = ImVec2(Image->Position.x + P0.x, Image->Position.y + P0.y);
                            ImVec2 Max = ImVec2(Min.x + Image->Size.x, Min.y + Image->Size.y);
                            DrawList->AddImage((void *)(intptr_t)Image->Tex, Min, Max);
                            
                            if((float)MouseX > Min.x && (float)MouseX < Max.x &&
                               (float)MouseY > Min.y && (float)MouseY < Max.y &&
                               
                               (float)MouseX > P0.x && (float)MouseX < P1.x &&
                               (float)MouseY > P0.y && (float)MouseY < P1.y &&
                               
                               !SettingProjectTitle)
                            {
                                MouseOverSomething = true;
                                if(MouseLDown)
                                {
                                    UnselectAll(Slide->TextCount, Slide->Texts, Slide->ImageCount, Slide->Images);
                                    Image->Selected = true;
                                }
                                else if(!Image->Selected)
                                {
                                    DrawList->AddRect(Min, Max, IM_COL32(0x69, 0x69, 0x69, 120), 0, 0, 1);
                                }
                            }
                            
                            if(Image->Selected)
                            {
                                DrawList->AddRect(Min, Max, IM_COL32(0, 0, 0, 255), 0, 0, 3);
                            }
                        }
                        
                        if(MouseLDown && !MouseOverSomething &&
                           (float)MouseX > P0.x && (float)MouseX < P1.x &&
                           (float)MouseY > P0.y && (float)MouseY < P1.y)
                        {
                            UnselectAll(Slide->TextCount, Slide->Texts, Slide->ImageCount, Slide->Images);
                        }
                    }
                    ImGui::PopClipRect();
                    
                    ImGui::Text("Slide Background Color");
                    ImColor NewColor = ImGui::ColorConvertU32ToFloat4(Slide->BGColor);
                    ImGui::ColorEdit3("##slide_color", (float *)&NewColor, 0);
                    Slide->BGColor = ImGui::ColorConvertFloat4ToU32(NewColor);
                    
                    int SelectedTextIndex = FindSelectedText(Slide->TextCount, Slide->Texts);
                    int SelectedImageIndex = FindSelectedImage(Slide->ImageCount, Slide->Images);
                    if(SelectedTextIndex >= 0)
                    {
                        ss_placed_text *SelectedText = &Slide->Texts[SelectedTextIndex];
                        ImGui::Text("Edit Text");
                        char RegText[SS_TEXT_LENGTH];
                        wcstombs(RegText, SelectedText->Text, SS_TEXT_LENGTH);
                        ImGui::InputText("##selected_text_input_t", RegText, SS_TEXT_LENGTH);
                        mbstowcs(SelectedText->Text, RegText, SS_TEXT_LENGTH);
                        
                        ImGui::DragFloat("##selected_text_input_pos_x",
                                         &SelectedText->Position.x,
                                         1.0f, 0, SLIDE_WIDTH, "X:%.0f", 0);
                        ImGui::DragFloat("##selected_text_input_pos_y",
                                         &SelectedText->Position.y,
                                         1.0f, 0, SLIDE_HEIGHT, "Y:%.0f", 0);
                        
                        ImColor TextColor = ImGui::ColorConvertU32ToFloat4(SelectedText->Color);
                        ImGui::ColorEdit3("##selected_text_input_color",
                                          (float *)&TextColor, 0);
                        SelectedText->Color = ImGui::ColorConvertFloat4ToU32(TextColor);
                        
                        ImGui::DragFloat("##selected_text_input_size",
                                         &SelectedText->Size, 0.5f,
                                         2.0f, 100.0f, "Size:%.1f", 0);
                        
                        if(ImGui::Button("Delete"))
                            DeleteText(&Slide->TextCount, (ss_placed_text **)Slide->Texts, SelectedTextIndex);
                    }
                    else if(SelectedImageIndex >= 0)
                    {
                        ss_placed_image *SelectedImage = &Slide->Images[SelectedImageIndex];
                        ImGui::Text("Edit Image");
                        char ImagePath[MAX_PATH];
                        wcstombs(ImagePath, SelectedImage->FilePath, MAX_PATH);
                        char ImagePathToShow[MAX_PATH];
                        sprintf(ImagePathToShow, "Image Path: %s", ImagePath);
                        ImGui::Text(ImagePathToShow);
                        ImGui::SameLine();
                        if(ImGui::Button("Load New Image File"))
                        {
                            OpenImage(&State, SelectedImage);
                        }
                        
                        ImGui::DragFloat("##selected_image_input_pos_x",
                                         &SelectedImage->Position.x,
                                         1.0f, 0, SLIDE_WIDTH - 5, "X:%.0f", 0);
                        ImGui::DragFloat("##selected_image_input_pos_y",
                                         &SelectedImage->Position.y,
                                         1.0f, 0, SLIDE_HEIGHT - 5, "Y:%.0f", 0);
                        
                        if(ImGui::Button("Lock Ratio"))
                        {
                            SelectedImage->SizeRatioLocked = !SelectedImage->SizeRatioLocked;
                            SelectedImage->SizeRatio = SelectedImage->Size.x / SelectedImage->Size.y;
                        }
                        if(SelectedImage->SizeRatioLocked)
                        {
                            ImGui::SameLine();
                            ImGui::Text("Locked");
                        }
                        
                        float NewWidth = SelectedImage->Size.x;
                        float NewHeight = SelectedImage->Size.y;
                        ImGui::DragFloat("##selcted_image_input_size_x",
                                         &NewWidth, 1.0f, 0, SLIDE_WIDTH * 3,
                                         "Width:%.0f", 0);
                        ImGui::DragFloat("##selcted_image_input_size_y",
                                         &NewHeight, 1.0f, 0, SLIDE_HEIGHT * 3,
                                         "Height:%.0f", 0);
                        
                        if(SelectedImage->SizeRatioLocked)
                        {
                            float DifferenceWidth = NewWidth - SelectedImage->Size.x;
                            float DifferenceHeight = NewHeight - SelectedImage->Size.y;
                            
                            if(DifferenceWidth != 0)
                            {
                                NewHeight += DifferenceWidth / SelectedImage->SizeRatio;
                            }
                            else if(DifferenceHeight != 0)
                            {
                                NewWidth += DifferenceHeight * SelectedImage->SizeRatio;
                            }
                        }
                        
                        SelectedImage->Size = ImVec2(NewWidth, NewHeight);
                        
                        if(ImGui::Button("Delete"))
                            DeleteImage(&Slide->ImageCount, (ss_placed_image **)Slide->Images, SelectedImageIndex);
                    }
                }
            }
            ImGui::End();
        }
        
        ImGui::Render();
        glViewport(0, 0, (int)IO.DisplaySize.x, (int)IO.DisplaySize.y);
        glClearColor(ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w,
                     ClearColor.z * ClearColor.w, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(Window);
    }
    
    WriteConfig(&State, State.ConfigFileName);
    Save(&State, State.SaveFileName);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    SDL_GL_DeleteContext(GLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
    SDL_free(State.Slides);
    
    return(0);
}

static bool
ProcessKey(ss_state *State, SDL_Keysym Sym, bool Pressed, bool *SettingProjectTitle)
{
    if(Pressed)
    {
        if(Sym.sym == SDLK_ESCAPE)
        {
            return(true);
        }
        
        if(Sym.sym == SDLK_b && Sym.mod & KMOD_CTRL)
        {
            Compile(State);
        }
        
        if(Sym.sym == SDLK_s && Sym.mod & KMOD_CTRL)
        {
            if(wcscmp(State->SaveFileName, L"\0"))
            {
                Save(State, State->SaveFileName);
            }
        }
        
        if(Sym.sym == SDLK_n && Sym.mod & KMOD_CTRL)
        {
            if(Sym.mod & KMOD_SHIFT)
            {
                NewSlide(State);
            }
            else
            {
                *SettingProjectTitle = true;
                NewProject(State, State->SaveFileName);
            }
        }
        
        if(Sym.sym == SDLK_o && Sym.mod & KMOD_CTRL)
        {
            OpenProject(State, State->SaveFileName);
        }
    }
    
    return(false);
}