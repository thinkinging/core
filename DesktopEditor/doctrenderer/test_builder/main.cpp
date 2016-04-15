#include "../docbuilder.h"

#ifdef LINUX
#include "../../common/File.h"
#endif

#define OFFICESTUDIO_FILE_DOCUMENT					0x0040
#define OFFICESTUDIO_FILE_DOCUMENT_DOCX				OFFICESTUDIO_FILE_DOCUMENT + 0x0001
#define OFFICESTUDIO_FILE_DOCUMENT_DOC				OFFICESTUDIO_FILE_DOCUMENT + 0x0002
#define OFFICESTUDIO_FILE_DOCUMENT_ODT				OFFICESTUDIO_FILE_DOCUMENT + 0x0003
#define OFFICESTUDIO_FILE_DOCUMENT_RTF				OFFICESTUDIO_FILE_DOCUMENT + 0x0004
#define OFFICESTUDIO_FILE_DOCUMENT_TXT				OFFICESTUDIO_FILE_DOCUMENT + 0x0005
#define OFFICESTUDIO_FILE_DOCUMENT_HTML				OFFICESTUDIO_FILE_DOCUMENT + 0x0006
#define OFFICESTUDIO_FILE_DOCUMENT_MHT				OFFICESTUDIO_FILE_DOCUMENT + 0x0007
#define OFFICESTUDIO_FILE_DOCUMENT_EPUB				OFFICESTUDIO_FILE_DOCUMENT + 0x0008
#define OFFICESTUDIO_FILE_DOCUMENT_FB2				OFFICESTUDIO_FILE_DOCUMENT + 0x0009
#define OFFICESTUDIO_FILE_DOCUMENT_MOBI				OFFICESTUDIO_FILE_DOCUMENT + 0x000a

#define OFFICESTUDIO_FILE_PRESENTATION				0x0080
#define OFFICESTUDIO_FILE_PRESENTATION_PPTX			OFFICESTUDIO_FILE_PRESENTATION + 0x0001
#define OFFICESTUDIO_FILE_PRESENTATION_PPT			OFFICESTUDIO_FILE_PRESENTATION + 0x0002
#define OFFICESTUDIO_FILE_PRESENTATION_ODP			OFFICESTUDIO_FILE_PRESENTATION + 0x0003
#define OFFICESTUDIO_FILE_PRESENTATION_PPSX			OFFICESTUDIO_FILE_PRESENTATION + 0x0004

#define OFFICESTUDIO_FILE_SPREADSHEET				0x0100
#define OFFICESTUDIO_FILE_SPREADSHEET_XLSX			OFFICESTUDIO_FILE_SPREADSHEET + 0x0001
#define OFFICESTUDIO_FILE_SPREADSHEET_XLS			OFFICESTUDIO_FILE_SPREADSHEET + 0x0002
#define OFFICESTUDIO_FILE_SPREADSHEET_ODS			OFFICESTUDIO_FILE_SPREADSHEET + 0x0003
#define OFFICESTUDIO_FILE_SPREADSHEET_CSV			OFFICESTUDIO_FILE_SPREADSHEET + 0x0004

#define OFFICESTUDIO_FILE_CROSSPLATFORM				0x0200
#define OFFICESTUDIO_FILE_CROSSPLATFORM_PDF			OFFICESTUDIO_FILE_CROSSPLATFORM + 0x0001
#define OFFICESTUDIO_FILE_CROSSPLATFORM_DJVU		OFFICESTUDIO_FILE_CROSSPLATFORM + 0x0003
#define OFFICESTUDIO_FILE_CROSSPLATFORM_XPS			OFFICESTUDIO_FILE_CROSSPLATFORM + 0x0004

#include <string>

#ifdef WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    if (argc <= 0)
        return 0;

#ifdef WIN32
    std::wstring sBuildFile(argv[argc - 1]);
#else
    std::string sBuildFileA(argv[argc - 1]);
    std::wstring sBuildFile = NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)sBuildFileA.c_str(), (LONG)sBuildFileA.length());
#endif

    NSDoctRenderer::CDocBuilder::Initialize();

    if (true)
    {
        NSDoctRenderer::CDocBuilder oBuilder(true);
        oBuilder.Run(sBuildFile.c_str());
    }

    NSDoctRenderer::CDocBuilder::Dispose();

    return 0;
}
