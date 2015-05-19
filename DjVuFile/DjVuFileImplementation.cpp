#include "DjVuFileImplementation.h"

#include "../DesktopEditor/common/File.h"
#include "../DesktopEditor/common/Directory.h"
#include "../DesktopEditor/graphics/IRenderer.h"
#include "../DesktopEditor/raster/BgraFrame.h"
#include "../DesktopEditor/graphics/Image.h"
#include "../DesktopEditor/common/String.h"

#include "../DesktopEditor/fontengine/FontManager.h"
#include "../DesktopEditor/fontengine/ApplicationFonts.h"
#include "../DesktopEditor/raster/BgraFrame.h"
#include "../DesktopEditor/graphics/GraphicsRenderer.h"

#define VER_DPI		96
#define HOR_DPI		96

#include <vector>

namespace NSDjvu
{
	static GUTF8String MakeUTF8String(const std::wstring& wsText)
	{
        std::string sText = NSFile::CUtf8Converter::GetUtf8StringFromUnicode(wsText);
        GUTF8String utf8String(sText.c_str());
        return utf8String;
	}
	static CString MakeCString(GUTF8String& strText)
	{
		std::string sString(strText.getbuf());
		std::wstring wsString = NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)sString.c_str(), sString.length());
		return CString(wsString.c_str());
	}
	static int     GetInteger(const std::wstring& wsString)
	{
		if (wsString.size() < 1)
			return 0;

		try
		{
			return _ttoi(wsString.c_str());
		}
		catch (...)
		{
		}

		try
		{
			return static_cast<int>(_wtoi64(wsString.c_str()));
		}
		catch (...)
		{
			return 0;
		}
	}
}

CDjVuFileImplementation::CDjVuFileImplementation()
{
	m_pDoc = NULL;
	std::wstring wsTempPath = NSFile::CFileBinary::GetTempPathW();
	wsTempPath += L"DJVU\\";
	m_wsTempDirectory = wsTempPath;
	NSDirectory::CreateDirectory(m_wsTempDirectory);
}
CDjVuFileImplementation::~CDjVuFileImplementation()
{
	NSDirectory::DeleteDirectory(m_wsTempDirectory);
}
bool               CDjVuFileImplementation::LoadFromFile(const std::wstring& wsSrcFileName, const std::wstring& wsXMLOptions)
{
	m_pDoc = NULL;
	try
	{
		GUTF8String utf8;
		GURL url = GURL::Filename::UTF8(NSDjvu::MakeUTF8String(wsSrcFileName));
		m_pDoc = DjVuDocument::create(url);
		m_pDoc->wait_get_pages_num();
	}
	catch (...)
	{
		return false;
	}

	return true;
}
void               CDjVuFileImplementation::Close()
{
}
std::wstring       CDjVuFileImplementation::GetTempDirectory() const
{
	return m_wsTempDirectory;
}
void               CDjVuFileImplementation::SetTempDirectory(const std::wstring& wsDirectory)
{
	NSDirectory::DeleteDirectory(m_wsTempDirectory);

	m_wsTempDirectory = wsDirectory;
	m_wsTempDirectory += L"\\DJVU\\";
	NSDirectory::CreateDirectory(m_wsTempDirectory);
}
int                CDjVuFileImplementation::GetPagesCount() const
{
	if (!m_pDoc)
		return 0;

	return m_pDoc->get_pages_num();
}
void               CDjVuFileImplementation::GetPageInfo(int nPageIndex, double* pdWidth, double* pdHeight, double* pdDpiX, double* pdDpiY) const
{
	if (!m_pDoc)
	{
		*pdWidth  = 0;
		*pdHeight = 0;

		*pdDpiX = 96;
		*pdDpiY = 96;
	}

	GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);

	pPage->wait_for_complete_decode();
	pPage->set_rotate(0);

	*pdWidth = pPage->get_real_width();
	*pdHeight = pPage->get_real_height();

	*pdDpiX = pPage->get_dpi();
	*pdDpiY = pPage->get_dpi();
}
void               CDjVuFileImplementation::DrawPageOnRenderer(IRenderer* pRenderer, int nPageIndex, bool* pBreak)
{
	if (!m_pDoc)
		return;

	try
	{
		GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);
		pPage->wait_for_complete_decode();
		pPage->set_rotate(0);

		long lRendererType = c_nUnknownRenderer;
		pRenderer->get_Type(&lRendererType);
		if (false)//c_nGrRenderer == lRendererType)
		{
			CreateGrFrame(pRenderer, pPage, pBreak);
		}
		else
		{
			XmlUtils::CXmlNode text = ParseText(pPage);
			CreateFrame(pRenderer, pPage, nPageIndex, text);
		}
	}
	catch (...)
	{
		// ����� ��������
	}
}
void               CDjVuFileImplementation::ConvertToRaster(CApplicationFonts* pAppFonts, int nPageIndex, const std::wstring& wsDstPath, int nImageType)
{
	CFontManager *pFontManager = pAppFonts->GenerateFontManager();
	CFontsCache* pFontCache = new CFontsCache();
	pFontCache->SetStreams(pAppFonts->GetStreams());
	pFontManager->SetOwnerCache(pFontCache);

	CGraphicsRenderer oRenderer;
	oRenderer.SetFontManager(pFontManager);

	double dPageDpiX, dPageDpiY;
	double dWidth, dHeight;
	GetPageInfo(nPageIndex, &dWidth, &dHeight, &dPageDpiX, &dPageDpiX);

	int nWidth  = (int)dWidth * 96 / dPageDpiX;
	int nHeight = (int)dHeight * 96 / dPageDpiX;

	BYTE* pBgraData = new BYTE[nWidth * nHeight * 4];
	if (!pBgraData)
		return;

	memset(pBgraData, 0xff, nWidth * nHeight * 4);
	CBgraFrame oFrame;
	oFrame.put_Data(pBgraData);
	oFrame.put_Width(nWidth);
	oFrame.put_Height(nHeight);
	oFrame.put_Stride(-4 * nWidth);

	oRenderer.CreateFromBgraFrame(&oFrame);
	oRenderer.SetSwapRGB(false);
	oRenderer.put_Width(dWidth);
	oRenderer.put_Height(dHeight);

	bool bBreak = false;
	DrawPageOnRenderer(&oRenderer, nPageIndex, &bBreak);

	oFrame.SaveFile(wsDstPath, nImageType);
	RELEASEINTERFACE(pFontManager);
}
void               CDjVuFileImplementation::CreateFrame(IRenderer* pRenderer, GP<DjVuImage>& pPage, int nPage, XmlUtils::CXmlNode& text)
{
	int nWidth	= pPage->get_real_width();
	int nHeight	= pPage->get_real_height();

	int nDpi = pPage->get_dpi();

	double dPixToMM = 25.4;

	double dRendDpiX = 0;
	double dRendDpiY = 0;

	pRenderer->get_DpiX(&dRendDpiX);
	pRenderer->get_DpiY(&dRendDpiY);

	if (0 >= dRendDpiX)
		dRendDpiX = 72.0;
	if (0 >= dRendDpiY)
		dRendDpiY = 72.0;

	double dRendWidth = 0;
	double dRendHeight = 0;

	pRenderer->get_Width(&dRendWidth);
	pRenderer->get_Height(&dRendHeight);

	if (0 >= dRendWidth)
		dRendWidth = 200;
	if (0 >= dRendHeight)
		dRendHeight = 300;

	LONG lImageWidth	= (LONG)(dRendDpiX * dRendWidth / dPixToMM);
	LONG lImageHeight	= (LONG)(dRendDpiY * dRendHeight / dPixToMM);

	long lRendererType = c_nUnknownRenderer;
	pRenderer->get_Type(&lRendererType);
	if (c_nPDFWriter == lRendererType)
	{
		lImageWidth	 = pPage->get_real_width();
		lImageHeight = pPage->get_real_height();
	}
	else if (c_nHtmlRendrerer == lRendererType)
	{
		// TODO: ����� ����������� ������� 
		// pRenderer->GetMaxImageSize();
		//VARIANT var;
		//renderer->GetAdditionalParam(L"MaxImageSize", &var);
		//LONG lMaxWidth = var.lVal;
		//if ((lImageWidth > lMaxWidth) || (lImageHeight > lMaxWidth))
		//{
		//	double dAspect = (double)(lImageWidth) / lImageHeight;
		//	if (lImageWidth > lImageHeight)
		//	{
		//		lImageWidth  = lMaxWidth;
		//		lImageHeight = (LONG)(lImageHeight / dAspect);
		//	}
		//	else
		//	{
		//		lImageHeight = lMaxWidth;
		//		lImageWidth  = (LONG)(dAspect * lImageHeight);
		//	}
		//}
	}

	BYTE* pBufferDst = new BYTE[4 * lImageHeight * lImageWidth];
	if (!pBufferDst)
		return;

	Aggplus::CImage oImage;
	oImage.Create(pBufferDst, lImageWidth, lImageHeight, 4 * lImageWidth);
	if (pPage->is_legal_photo() || pPage->is_legal_compound())
	{
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		BYTE* pBuffer = pBufferDst;
		for (int j = lImageHeight - 1; j >= 0; --j)
		{
			GPixel* pLine = pImage->operator [](j);

			for (int i = 0; i < lImageWidth; ++i, pBuffer += 4, ++pLine)
			{
				pBuffer[0] = pLine->b;
				pBuffer[1] = pLine->g;
				pBuffer[2] = pLine->r;
				pBuffer[3] = 255;
			}
		}
	}
	else if (pPage->is_legal_bilevel())
	{
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);
		int nPaletteEntries = pBitmap->get_grays();

		DWORD* palette = new DWORD[nPaletteEntries];

		// Create palette for the bitmap
		int color = 0xff0000;
		int decrement = color / (nPaletteEntries - 1);
		for (int i = 0; i < nPaletteEntries; ++i)
		{
			BYTE level = (BYTE)(color >> 16);
			palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
			color -= decrement;
		}

		DWORD* pBuffer = (DWORD*)pBufferDst;
		for (int j = lImageHeight - 1; j >= 0; --j)
		{
			BYTE* pLine = pBitmap->operator [](j);

			for (int i = 0; i < lImageWidth; ++i, ++pBuffer, ++pLine)
			{
				if (*pLine < nPaletteEntries)
				{
					*pBuffer = palette[*pLine];
				}
				else
				{
					*pBuffer = palette[0];
				}
			}
		}

		RELEASEARRAYOBJECTS(palette);
	}
	else
	{
		// ����� �����??
		//memset(pBufferDst, 0xFF, 4 * lImageWidth * lImageHeight);
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		if (NULL != pImage)
		{
			BYTE* pBuffer = pBufferDst;
			for (int j = lImageHeight - 1; j >= 0; --j)
			{
				GPixel* pLine = pImage->operator [](j);

				for (int i = 0; i < lImageWidth; ++i, pBuffer += 4, ++pLine)
				{
					pBuffer[0] = pLine->b;
					pBuffer[1] = pLine->g;
					pBuffer[2] = pLine->r;
					pBuffer[3] = 255;
				}
			}
		}
		else
		{
			GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);

			if (NULL != pBitmap)
			{
				int nPaletteEntries = pBitmap->get_grays();

				DWORD* palette = new DWORD[nPaletteEntries];

				// Create palette for the bitmap
				int color = 0xff0000;
				int decrement = color / (nPaletteEntries - 1);
				for (int i = 0; i < nPaletteEntries; ++i)
				{
					BYTE level = (BYTE)(color >> 16);
					palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
					color -= decrement;
				}

				DWORD* pBuffer = (DWORD*)pBufferDst;
				for (int j = lImageHeight - 1; j >= 0; --j)
				{
					BYTE* pLine = pBitmap->operator [](j);

					for (int i = 0; i < lImageWidth; ++i, ++pBuffer, ++pLine)
					{
						if (*pLine < nPaletteEntries)
						{
							*pBuffer = palette[*pLine];
						}
						else
						{
							*pBuffer = palette[0];
						}
					}
				}

				RELEASEARRAYOBJECTS(palette);
			}
		}
	}

	pRenderer->BeginCommand(c_nPageType);

	if (c_nGrRenderer != lRendererType && c_nHtmlRendrerer != lRendererType && c_nHtmlRendrerer2 != lRendererType)
	{
		TextToRenderer(pRenderer, text, dPixToMM / nDpi);
	}

	pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dRendWidth, dRendHeight);
	pRenderer->EndCommand(c_nPageType);
}
void               CDjVuFileImplementation::CreateGrFrame(IRenderer* pRenderer, GP<DjVuImage>& pPage, bool* pBreak)
{
	int nWidth	= pPage->get_real_width();
	int nHeight	= pPage->get_real_height();

	BYTE*	pBufferDst	 = NULL;
	LONG	lImageWidth	 = 0;
	LONG	lImageHeight = 0;

	// TODO: ����������� ��� ������������ ���������

	//VARIANT var;
	//renderer->GetAdditionalParam(L"Pixels", &var);
	//pBufferDst = (BYTE*)var.lVal;

	//renderer->GetAdditionalParam(L"PixelsWidth", &var);
	//lImageWidth  = var.lVal;
	//renderer->GetAdditionalParam(L"PixelsHeight", &var);
	//lImageHeight = var.lVal;

	volatile bool* pCancel = pBreak;

	if ((NULL != pCancel) && (TRUE == *pCancel))
		return;

	if (pPage->is_legal_photo() || pPage->is_legal_compound())
	{
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		BYTE* pBuffer = pBufferDst;
		for (int j = 0; j < lImageHeight; ++j)
		{
			GPixel* pLine = pImage->operator [](j);

			if ((NULL != pCancel) && (TRUE == *pCancel))
				return;

			for (int i = 0; i < lImageWidth; ++i, pBuffer += 4, ++pLine)
			{
				pBuffer[0] = pLine->b;
				pBuffer[1] = pLine->g;
				pBuffer[2] = pLine->r;
				pBuffer[3] = 255;
			}
		}
	}
	else if (pPage->is_legal_bilevel())
	{
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);
		int nPaletteEntries = pBitmap->get_grays();

		DWORD* palette = new DWORD[nPaletteEntries];

		// Create palette for the bitmap
		int color = 0xff0000;
		int decrement = color / (nPaletteEntries - 1);
		for (int i = 0; i < nPaletteEntries; ++i)
		{
			BYTE level = (BYTE)(color >> 16);
			palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
			color -= decrement;
		}

		DWORD* pBuffer = (DWORD*)pBufferDst;
		for (int j = 0; j < lImageHeight; ++j)
		{
			BYTE* pLine = pBitmap->operator [](j);

			if ((NULL != pCancel) && (TRUE == *pCancel))
				return;

			for (int i = 0; i < lImageWidth; ++i, ++pBuffer, ++pLine)
			{
				if (*pLine < nPaletteEntries)
				{
					*pBuffer = palette[*pLine];
				}
				else
				{
					*pBuffer = palette[0];
				}
			}
		}

		RELEASEARRAYOBJECTS(palette);
	}
	else
	{
		// ����� �����??
		//memset(pBufferDst, 0xFF, 4 * lImageWidth * lImageHeight);
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		if (NULL != pImage)
		{
			BYTE* pBuffer = pBufferDst;
			for (int j = 0; j < lImageHeight; ++j)
			{
				GPixel* pLine = pImage->operator [](j);

				if ((NULL != pCancel) && (TRUE == *pCancel))
					return;

				for (int i = 0; i < lImageWidth; ++i, pBuffer += 4, ++pLine)
				{
					pBuffer[0] = pLine->b;
					pBuffer[1] = pLine->g;
					pBuffer[2] = pLine->r;
					pBuffer[3] = 255;
				}
			}

			return;
		}

		GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);

		if (NULL != pBitmap)
		{
			int nPaletteEntries = pBitmap->get_grays();

			DWORD* palette = new DWORD[nPaletteEntries];

			// Create palette for the bitmap
			int color = 0xff0000;
			int decrement = color / (nPaletteEntries - 1);
			for (int i = 0; i < nPaletteEntries; ++i)
			{
				BYTE level = (BYTE)(color >> 16);
				palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
				color -= decrement;
			}

			DWORD* pBuffer = (DWORD*)pBufferDst;
			for (int j = 0; j < lImageHeight; ++j)
			{
				BYTE* pLine = pBitmap->operator [](j);

				if ((NULL != pCancel) && (TRUE == *pCancel))
					return;

				for (int i = 0; i < lImageWidth; ++i, ++pBuffer, ++pLine)
				{
					if (*pLine < nPaletteEntries)
					{
						*pBuffer = palette[*pLine];
					}
					else
					{
						*pBuffer = palette[0];
					}
				}
			}

			RELEASEARRAYOBJECTS(palette);
		}
	}
}
XmlUtils::CXmlNode CDjVuFileImplementation::ParseText(GP<DjVuImage> pPage)
{
	XmlUtils::CXmlNode paragraph;
	const GP<DjVuText> text(DjVuText::create());
	const GP<ByteStream> text_str(pPage->get_text());
	if (text_str)
	{
		text->decode(text_str);
		GUTF8String pageText = text->get_xmlText(pPage->get_height());
		XmlUtils::CXmlNode hiddenText;
		XmlUtils::CXmlNode pageColumn;
		XmlUtils::CXmlNode region;
		hiddenText.FromXmlString(NSDjvu::MakeCString(pageText));
		hiddenText.GetNode(_T("PAGECOLUMN"), pageColumn);
		pageColumn.GetNode(_T("REGION"), region);
		region.GetNode(_T("PARAGRAPH"), paragraph);
	}
	return paragraph;
}
void               CDjVuFileImplementation::TextToRenderer(IRenderer* pRenderer, XmlUtils::CXmlNode oTextNode, double dKoef, bool isView)
{
	// �������� ����� ������ (����� ����������� �� ����� �����)
	pRenderer->put_FontName(L"AVSEmptyFont");
	CString csText = oTextNode.GetXml();
	XmlUtils::CXmlNodes oLinesNodes;
	oTextNode.GetNodes(L"LINE", oLinesNodes);
	for (int nLineIndex = 0; nLineIndex < oLinesNodes.GetCount(); ++nLineIndex)
	{
		XmlUtils::CXmlNode oLineNode;
		oLinesNodes.GetAt(nLineIndex, oLineNode);
		XmlUtils::CXmlNodes oWordsNodes;
		oLineNode.GetNodes(L"WORD", oWordsNodes);
		for (int nWordIndex = 0; nWordIndex < oWordsNodes.GetCount(); ++nWordIndex)
		{
			XmlUtils::CXmlNode oWordNode;
			oWordsNodes.GetAt(nWordIndex, oWordNode);
			CString csWord   = oWordNode.GetText();
			CString csCoords = oWordNode.GetAttribute(L"coords");
			double arrCoords[4];
			ParseCoords(csCoords.GetBuffer(), arrCoords, dKoef);
			DrawText(pRenderer, arrCoords, csWord.GetBuffer());
		}
	}
}
void               CDjVuFileImplementation::DrawText(IRenderer* pRenderer, double* pdCoords, const std::wstring& wsText)
{
	pRenderer->put_FontSize(pdCoords[1] - pdCoords[3]);
	pRenderer->CommandDrawText(wsText,
							   (float)(pdCoords[0]),
							   (float)(pdCoords[3]),
							   (float)(pdCoords[2] - pdCoords[0]),
							   (float)(pdCoords[1] - pdCoords[3]),
							   (float)(pdCoords[1] - pdCoords[3]));
}
void               CDjVuFileImplementation::ParseCoords(const std::wstring& wsCoordsStr, double* pdCoords, double dKoef)
{
	std::vector<std::wstring> vCoords = NSString::Split(wsCoordsStr, L',');
	if (vCoords.size() >= 4)
	{
		for (int nIndex = 0; nIndex < 4; nIndex++)
		{
			pdCoords[nIndex] = NSDjvu::GetInteger(vCoords.at(nIndex)) * dKoef;
		}
	}
}
