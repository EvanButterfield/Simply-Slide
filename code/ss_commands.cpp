static int
FindSelectedText(int TextCount, ss_placed_text *Texts)
{
    int Result = -1;
    
    for(int TextIndex = 0;
        TextIndex < TextCount;
        ++TextIndex)
    {
        if(Texts[TextIndex].Selected)
        {
            Result = TextIndex;
            break;
        }
    }
    
    return(Result);
}

static void
UnselectAllText(int TextCount, ss_placed_text *Texts)
{
    for(int TextIndex = 0;
        TextIndex < TextCount;
        ++TextIndex)
    {
        Texts[TextIndex].Selected = false;
    }
}

static int
FindSelectedImage(int ImageCount, ss_placed_image *Images)
{
    int Result = -1;
    
    for(int ImageIndex = 0;
        ImageIndex < ImageCount;
        ++ImageIndex)
    {
        if(Images[ImageIndex].Selected)
        {
            Result = ImageIndex;
            break;
        }
    }
    
    return(Result);
}

static void
UnselectAllImages(int ImageCount, ss_placed_image *Images)
{
    for(int ImageIndex = 0;
        ImageIndex < ImageCount;
        ++ImageIndex)
    {
        Images[ImageIndex].Selected = false;
    }
}

static void
UnselectAll(int TextCount, ss_placed_text *Texts,
            int ImageCount, ss_placed_image *Images)
{
    UnselectAllText(TextCount, Texts);
    UnselectAllImages(ImageCount, Images);
}

static void
NewSlide(ss_state *State)
{
    ss_slide *SlideCopy = 0;
    ALLOC_STRUCT(State->SlideCount, &SlideCopy, ss_slide);
    memcpy(SlideCopy, State->Slides, State->SlideCount * sizeof(ss_slide));
    SDL_free(State->Slides);
    
    ++State->SlideCount;
    ++State->SlideIndex;
    ALLOC_STRUCT(State->SlideCount, &State->Slides, ss_slide);
    memcpy(State->Slides, SlideCopy, (State->SlideCount - 1) * sizeof(ss_slide));
    SDL_free(SlideCopy);
    
    memset(&State->Slides[State->SlideCount - 1], 0, sizeof(ss_slide));
    State->Slides[State->SlideCount - 1].BGColor = IM_COL32(255, 255, 255, 255);
}

static void
DeleteSlide(ss_state *State, int SlideToDeleteIndex)
{
    ss_slide *SlideCopy = 0;
    ALLOC_STRUCT(State->SlideCount, &SlideCopy, ss_slide);
    memcpy(SlideCopy, State->Slides, State->SlideCount * sizeof(ss_slide));
    SDL_free(State->Slides);
    
    for(int SlideIndex = SlideToDeleteIndex + 1;
        SlideIndex < State->SlideCount;
        ++SlideIndex)
    {
        ss_slide *SlideToCopy = &SlideCopy[SlideIndex];
        ss_slide *SlideToCopyTo = &SlideCopy[SlideIndex - 1];
        memcpy(SlideToCopyTo, SlideToCopy, sizeof(ss_slide));
    }
    
    --State->SlideCount;
    --State->SlideIndex;
    ALLOC_STRUCT(State->SlideCount, &State->Slides, ss_slide);
    memcpy(State->Slides, SlideCopy, State->SlideCount * sizeof(ss_slide));
    SDL_free(SlideCopy);
}

static void
Save(ss_state *State, LPCWSTR SaveFileName)
{
    WriteProjectToIni(State, SaveFileName);
}

static void
AddText(ss_slide *Slide, LPCWSTR Text, ImVec2 Position)
{
    ++Slide->TextCount;
    ss_placed_text NewText = {};
    wcsncpy(NewText.Text, Text, SS_TEXT_LENGTH);
    NewText.Position = Position;
    
    memcpy(&Slide->Texts[Slide->TextCount - 1],
           &NewText, sizeof(ss_placed_text));
    
    Slide->Texts[Slide->TextCount - 1].Color = IM_COL32(0, 0, 0, 255);
    Slide->Texts[Slide->TextCount - 1].Size = 12.0f;
}

static void
DeleteText(int *TextCount, ss_placed_text **Texts, int TextIndex)
{
    ss_placed_text *TextsCopy = 0;
    ALLOC_STRUCT(*TextCount, &TextsCopy, ss_placed_text);
    memcpy(TextsCopy, Texts, *TextCount * sizeof(ss_placed_text));
    SDL_free(Texts);
    
    {
        ss_placed_text *TextToDelete = &TextsCopy[TextIndex];
        ss_placed_text *LastText = &TextsCopy[*TextCount - 1];
        memcpy(TextToDelete, LastText, sizeof(ss_placed_text));
    }
    
    *TextCount -= 1;
    ALLOC_STRUCT(*TextCount, Texts, ss_placed_text);
    memcpy(Texts, TextsCopy, *TextCount * sizeof(ss_placed_text));
    SDL_free(TextsCopy);
}

static void
AddImage(ss_slide *Slide, GLuint Tex, LPCWSTR ImagePath, ImVec2 Position, ImVec2 Size)
{
    ++Slide->ImageCount;
    ss_placed_image NewImage = {};
    wcsncpy(NewImage.FilePath, ImagePath, MAX_PATH);
    NewImage.Tex = Tex;
    NewImage.Position = Position;
    NewImage.Size = Size;
    
    memcpy(&Slide->Images[Slide->ImageCount - 1],
           &NewImage, sizeof(ss_placed_image));
}

static void
DeleteImage(int *ImageCount, ss_placed_image **Images, int ImageIndex)
{
    ss_placed_image *ImagesCopy = 0;
    ALLOC_STRUCT(*ImageCount, &ImagesCopy, ss_placed_image);
    memcpy(ImagesCopy, Images, *ImageCount * sizeof(ss_placed_image));
    SDL_free(Images);
    
    {
        ss_placed_image *ImageToDelete = &ImagesCopy[ImageIndex];
        ss_placed_image *LastImage = &ImagesCopy[*ImageCount - 1];
        memcpy(ImageToDelete, LastImage, sizeof(ss_placed_image));
    }
    
    *ImageCount -= 1;
    ALLOC_STRUCT(*ImageCount, Images, ss_placed_image);
    memcpy(Images, ImagesCopy, *ImageCount * sizeof(ss_placed_image));
    SDL_free(ImagesCopy);
}

static bool
OpenProject(ss_state *State, WCHAR *FileName)
{
    if(wcscmp(State->SaveFileName, L"\0"))
    {
        Save(State, State->SaveFileName);
    }
    
    OPENFILENAMEW File = {};
    File.lStructSize = sizeof(OPENFILENAME);
    File.hInstance = State->Instance;
    File.lpstrFilter = L"Simply Slide Files (*.ss)\0*.ss\0\0";
    File.lpstrFile = FileName;
    File.nMaxFile = MAX_PATH;
    File.lpstrTitle = L"Open Simply Slide Project";
    File.Flags = OFN_FILEMUSTEXIST;
    
    if(GetOpenFileNameW(&File) == 0)
        return(false);
    
    GetProjectFromIni(State);
    
    return(true);
}

static bool
NewProject(ss_state *State, WCHAR *FileName)
{
    OPENFILENAMEW File = {};
    File.lStructSize = sizeof(OPENFILENAME);
    File.hInstance = State->Instance;
    File.lpstrFilter = L"Simply Slide Files (*.ss)\0*.ss\0\0";
    File.lpstrFile = FileName;
    File.nMaxFile = MAX_PATH;
    File.lpstrTitle = L"Open Simply Slide Project";
    
    if(GetSaveFileNameW(&File) == 0)
        return(false);
    
    WCHAR NewRoboto[MAX_PATH];
    wcscpy(NewRoboto, FileName);
    PathCchRemoveFileSpec(NewRoboto, MAX_PATH);
    wcscat(NewRoboto, L"\\Roboto.ttf");
    char NewRobotoCStr[MAX_PATH];
    wcstombs(NewRobotoCStr, NewRoboto, MAX_PATH);
    
    WCHAR CurrentRoboto[MAX_PATH];
    wcscpy(CurrentRoboto, State->EXEPath);
    wcscat(CurrentRoboto, L"\\Roboto.ttf");
    char CurrentRobotoCStr[MAX_PATH];
    wcstombs(CurrentRobotoCStr, CurrentRoboto, MAX_PATH);
    
    CopyFile(CurrentRobotoCStr, NewRobotoCStr, false);
    
    return(true);
}

static void
GetEXEPath(WCHAR *Path)
{
    char EXEPath[MAX_PATH];
    GetModuleFileNameA(0, EXEPath, MAX_PATH);
    mbstowcs(Path, EXEPath, MAX_PATH);
    PathCchRemoveFileSpec(Path, MAX_PATH);
}

static bool
LoadTexture(char *FileName, GLuint *Texture, int *Width, int *Height)
{
    int ImageWidth = 0;
    int ImageHeight = 0;
    int Channels = 0;
    unsigned char *ImageData = stbi_load(FileName, &ImageWidth, &ImageHeight, &Channels, 4);
    if(ImageData == 0)
        return(false);
    
    GLuint GLTexture;
    glGenTextures(1, &GLTexture);
    glBindTexture(GL_TEXTURE_2D, GLTexture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ImageWidth, ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImageData);
    stbi_image_free(ImageData);
    
    *Texture = GLTexture;
    *Width = ImageWidth;
    *Height = ImageHeight;
    
    return(true);
}

static void
UnloadTexture(GLuint *Texture)
{
    glDeleteTextures(1, Texture);
}

static bool
OpenImage(ss_state *State, ss_placed_image *Image)
{
    OPENFILENAMEW File = {};
    File.lStructSize = sizeof(OPENFILENAMEW);
    File.hInstance = State->Instance;
    File.lpstrFilter = L"Basic Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0\0";
    File.lpstrFile = Image->FilePath;
    File.nMaxFile = MAX_PATH;
    File.lpstrTitle = L"Open Image";
    File.Flags = OFN_FILEMUSTEXIST;
    if(GetOpenFileNameW(&File) == 0)
        return(false);
    
    if(Image->Tex)
        UnloadTexture(&Image->Tex);
    
    char FilePathCStr[MAX_PATH];
    wcstombs(FilePathCStr, Image->FilePath, MAX_PATH);
    
    int ImageWidth = 0;
    int ImageHeight = 0;
    if(!LoadTexture(FilePathCStr, &Image->Tex, &ImageWidth, &ImageHeight))
        return(false);
    
    Image->Size = ImVec2((float)ImageWidth, (float)ImageHeight);
    
    WCHAR ImageFileName[MAX_PATH];
    wcscpy(ImageFileName, Image->FilePath);
    PathStripPathW(ImageFileName);
    char ImageFileNameCStr[MAX_PATH];
    wcstombs(ImageFileNameCStr, ImageFileName, MAX_PATH);
    
    WCHAR CurrentDirectory[MAX_PATH];
    wcscpy(CurrentDirectory, State->SaveFileName);
    PathCchRemoveFileSpec(CurrentDirectory, MAX_PATH);
    
    WCHAR NewFilePath[MAX_PATH];
    wcscpy(NewFilePath, CurrentDirectory);
    wcscat(NewFilePath, L"\\");
    wcscat(NewFilePath, ImageFileName);
    char NewFilePathCStr[MAX_PATH];
    wcstombs(NewFilePathCStr, NewFilePath, MAX_PATH);
    
    CopyFile(FilePathCStr, NewFilePathCStr, false);
    
    wcscpy(Image->FilePath, NewFilePath);
    
    return(true);
}

static void
Compile(ss_state *State)
{
    char *Top = 
        "<!DOCTYPE html>\n"
        "<html>\n";
    char *Bottom = "</html>\n";
    
    char Head[4096];
    sprintf(Head,
            "<head>\n"
            
            "<meta charset=\"UTF-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            "<title>%s</title>\n"
            
            "<style>\n"
            "@font-face {\n"
            "  font-family: MyRoboto;\n"
            "  src: url(Roboto.ttf);\n"
            "}\n"
            
            "body {\n"
            "  background-color: black;\n"
            "  font-family: MyRoboto;\n"
            "  margin: 0;\n"
            "  padding: 0;\n"
            "}\n"
            "div {\n"
            "  overflow: hidden;\n"
            "  position: absolute;\n"
            "  width: %ipx;\n"
            "  height: %ipx;\n"
            "}\n"
            "img {\n"
            "  position: absolute;\n"
            "  overflow: hidden;\n"
            "}\n"
            "p {\n"
            "  position: absolute;\n"
            "  overflow: hidden;\n"
            "  width: 100%%;\n"
            "}\n"
            "</style>\n"
            
            "</head>\n",
            State->ProjectName,
            SLIDE_WIDTH, SLIDE_HEIGHT);
    
    char Body[8192];
    strcpy(Body, "<body>\n\n");
    for(int SlideIndex = 0;
        SlideIndex < State->SlideCount;
        ++SlideIndex)
    {
        ss_slide *Slide = &State->Slides[SlideIndex];
        
        int R = (int)((Slide->BGColor >> IM_COL32_R_SHIFT) & 0xFF);
        int G = (int)((Slide->BGColor >> IM_COL32_G_SHIFT) & 0xFF);
        int B = (int)((Slide->BGColor >> IM_COL32_B_SHIFT) & 0xFF);
        
        char NewSlide[1024];
        sprintf(NewSlide,
                "<div id=\"slide%i\" style=\"background-color: rgb(%i, %i, %i);\">\n",
                SlideIndex, R, G, B);
        
        for(int TextIndex = 0;
            TextIndex < Slide->TextCount;
            ++TextIndex)
        {
            ss_placed_text *Text = &Slide->Texts[TextIndex];
            char String[SS_TEXT_LENGTH];
            wcstombs(String, Text->Text, SS_TEXT_LENGTH);
            
            int TextR = (int)((Text->Color >> IM_COL32_R_SHIFT) & 0xFF);
            int TextG = (int)((Text->Color >> IM_COL32_G_SHIFT) & 0xFF);
            int TextB = (int)((Text->Color >> IM_COL32_B_SHIFT) & 0xFF);
            
            int X = (int)Text->Position.x;
            int Y = (int)Text->Position.y;
            
            int FontSize = (int)Text->Size;
            
            char P[1024];
            sprintf(P,
                    "<p style=\"color: rgb(%i, %i, %i); font-size: %ipx; left: %ipx; top: %ipx;\">%s</p>\n",
                    TextR, TextG, TextB, FontSize, X, Y, String);
            
            strcat(NewSlide, P);
        }
        
        for(int ImageIndex = 0;
            ImageIndex < Slide->ImageCount;
            ++ImageIndex)
        {
            ss_placed_image *Image = &Slide->Images[ImageIndex];
            WCHAR FileName[MAX_PATH];
            wcscpy(FileName, Image->FilePath);
            PathStripPathW(FileName);
            char FileNameCStr[MAX_PATH];
            wcstombs(FileNameCStr, FileName, MAX_PATH);
            
            int X = (int)Image->Position.x;
            int Y = (int)Image->Position.y;
            
            int Width = (int)Image->Size.x;
            int Height = (int)Image->Size.y;
            
            char Img[1024];
            sprintf(Img,
                    "<img style=\"left: %ipx; top: %ipx\" src=\"%s\" width=\"%i\" height=\"%i\">",
                    X, Y, FileNameCStr, Width, Height);
            
            strcat(NewSlide, Img);
        }
        
        strcat(NewSlide, "</div>\n");
        strcat(Body, NewSlide);
    }
    
    char JS[4096];
    sprintf(JS,
            "\n<script>\n"
            "var SlideIndex = 0;\n"
            "var SlideCount = %i;\n"
            
            "for(let Index = 0;\n"
            "    Index < SlideCount;\n"
            "    ++Index) {\n"
            "  var Element = document.getElementById(\"slide\" + Index);\n"
            "  Element.style.visibility = \"hidden\";\n"
            "}\n"
            
            "document.getElementById(\"slide\" + SlideIndex).style.visibility = \"visible\";\n"
            
            "document.addEventListener(\'keydown\', function(event) {\n"
            "  if((event.keyCode == 13 || event.keyCode == 39) && SlideIndex + 1 < SlideCount)\n"
            "  {\n"
            "    document.getElementById(\"slide\" + SlideIndex).style.visibility = \"hidden\";\n"
            "    ++SlideIndex;\n"
            "    document.getElementById(\"slide\" + SlideIndex).style.visibility = \"visible\";\n"
            "  }\n"
            "  else if((event.keyCode == 8 || event.keyCode == 37) && SlideIndex - 1 >= 0)\n"
            "  {\n"
            "    document.getElementById(\"slide\" + SlideIndex).style.visibility = \"hidden\";\n"
            "    --SlideIndex;\n"
            "    document.getElementById(\"slide\" + SlideIndex).style.visibility = \"visible\";\n"
            "  }\n"
            "});\n"
            "</script>\n",
            State->SlideCount);
    strcat(Body, JS);
    
    strcat(Body, "\n</body>\n");
    
    char HTML[8192];
    strcpy(HTML, Top);
    strcat(HTML, Head);
    strcat(HTML, Body);
    strcat(HTML, Bottom);
    
    WCHAR HTMLFileName[MAX_PATH];
    wcscpy(HTMLFileName, State->SaveFileName);
    PathCchRemoveFileSpec(HTMLFileName, MAX_PATH);
    wcscat(HTMLFileName, L"\\presentation.html");
    char HTMLFileNameCStr[MAX_PATH];
    wcstombs(HTMLFileNameCStr, HTMLFileName, MAX_PATH);
    
    HANDLE HTMLFile = CreateFileA(HTMLFileNameCStr, GENERIC_WRITE,
                                  FILE_SHARE_WRITE|FILE_SHARE_READ, 0, OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, 0);
    if(HTMLFile != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        DWORD DataSize = (DWORD)strlen(HTML) * sizeof(char);
        WriteFile(HTMLFile, HTML, DataSize, &BytesWritten, 0);
    }
    CloseHandle(HTMLFile);
}