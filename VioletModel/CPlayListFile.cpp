#include "pch.h"
#include "CPlayListFile.h"


BOOL CPlayListFileReader::Open(PCWSTR pszFile)
{
	auto p = m_File.Open(pszFile, FILE_MAP_READ, PAGE_READONLY,
		eck::FCD_ONLYEXISTING, GENERIC_READ);
	if (!p)
		return FALSE;

	if (memcmp(p, "PNPL", 4) == 0)
	{
		m_pHeader0 = nullptr;
		m_pHeader1 = (LISTFILEHEADER_1*)p;
		return TRUE;
	}
	else if (memcmp(p, "QKPL", 4) == 0)
	{
		m_pHeader0 = (LISTFILEHEADER_0*)p;
		m_pHeader1 = nullptr;
		return TRUE;
	}
	else
		return FALSE;
}

int CPlayListFileReader::GetItemCount()
{
	if (m_pHeader1)
		return m_pHeader1->cItems;
	else if (m_pHeader0)
		return m_pHeader0->iCount;
	else
	{
		EckDbgBreak();
		return 0;
	}
}

void CPlayListFileReader::For(const FItemProcessor& fnProcessor)
{
	eck::CMemReader r{ nullptr };
	PCWSTR pszName, pszFile, pszTitle, pszArtist, pszAlbum, pszGenre;
	if (m_pHeader1) ECKLIKELY
	{
		r.SetPtr(m_pHeader1, m_File.GetFile().GetSize32());

		r += sizeof(LISTFILEHEADER_1);
		m_rsCreator.DupString((PCWSTR)r.Data(), m_pHeader1->cchCreator);
		r += eck::Cch2CbW(m_pHeader1->cchCreator);

		const LISTFILEITEM_1* pItem;

		EckCounter(m_pHeader1->cItems, i)
		{
			r.SkipPointer(pItem);

			pszName = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchName);
			pszFile = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchFile);
			pszTitle = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchTitle);
			pszArtist = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchArtist);
			pszAlbum = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchAlbum);
			pszGenre = (PCWSTR)r.Data();
			r += eck::Cch2CbW(pItem->cchGenre);
			fnProcessor(pItem, pszName, pszFile, pszTitle,
				pszArtist, pszAlbum, pszGenre);
		}
	}
	else if (m_pHeader0)
	{
		r.SetPtr(m_pHeader0, m_File.GetFile().GetSize32());

		r += sizeof(LISTFILEHEADER_0);
		LISTFILEITEM_1 Item1{};
		Item1.s.bNeedUpdated = TRUE;
		const LISTFILEITEM_0* pItem;
		EckCounter(m_pHeader0->iCount, i)
		{
			r.SkipPointer(pItem);
			Item1.s.bIgnore = eck::IsBitSet(pItem->uFlags, QKLIF_IGNORED);

			pszName = (PCWSTR)r.m_pMem;
			Item1.cchName = (int)wcslen(pszName);
			r += ((Item1.cchName + 1) * sizeof(WCHAR));

			pszFile = (PCWSTR)r.m_pMem;
			Item1.cchFile = (int)wcslen(pszFile);
			r += ((Item1.cchFile + 1) * sizeof(WCHAR));

			if (eck::IsBitSet(pItem->uFlags, QKLIF_BOOKMARK))
			{
				Item1.s.bBookmark = TRUE;
				r += sizeof(COLORREF);
				r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));
				r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));
			}
			else
			{
				r += sizeof(COLORREF) + sizeof(WCHAR) * 2;
				Item1.s.bBookmark = FALSE;
			}

			if (m_pHeader0->dwVer == QKLFVER_2)
				r += (((int)wcslen((PCWSTR)r.Data()) + 1) * sizeof(WCHAR));

			fnProcessor(&Item1, pszName, pszFile, nullptr, nullptr, nullptr, nullptr);
		}
	}
	else
		EckDbgBreak();
}

void CPlayListFileReader::ForBookmark(const FBookmarkProcessor& fnProcessor)
{
	eck::CMemReader r{ nullptr };
	PCWSTR pszName;
	if (m_pHeader1)
	{
#ifdef _DEBUG
		r.SetPtr(m_pHeader1, m_File.GetFile().GetSize32());
#else
		r.SetPtr(m_pHeader1, 0);
#endif
		r += m_pHeader1->ocbBookMark;
		const BOOKMARKHEADER* pHeader;
		const BOOKMARKITEM* pItem;
		r.SkipPointer(pHeader);
		EckCounter(pHeader->cBookmarks, i)
		{
			r.SkipPointer(pItem);
			pszName = (PCWSTR)r.m_pMem;
			r += ((pItem->cchName + 1) * sizeof(WCHAR));
			fnProcessor(pItem, pszName);
		}
	}
	else if (m_pHeader0)
	{
#ifdef _DEBUG
		r.SetPtr(m_pHeader0, m_File.GetFile().GetSize32());
#else
		r.SetPtr(m_pHeader0, 0);
#endif
		r += sizeof(LISTFILEHEADER_0);
		BOOKMARKITEM Item;
		const LISTFILEITEM_0* pItem;
		EckCounter(m_pHeader0->iCount, i)
		{
			r.SkipPointer(pItem);

			r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));
			r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));

			if (eck::IsBitSet(pItem->uFlags, QKLIF_BOOKMARK))
			{
				r >> Item.cr;

				pszName = (PCWSTR)r.m_pMem;
				Item.cchName = (int)wcslen(pszName);
				r += ((Item.cchName + 1) * sizeof(WCHAR));

				Item.idxItem = i;

				fnProcessor(&Item, pszName);

				r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));
			}
			else
				r += sizeof(COLORREF) + sizeof(WCHAR) * 2;

			if (m_pHeader0->dwVer == QKLFVER_2)
				r += ((wcslen((PCWSTR)r.m_pMem) + 1) * sizeof(WCHAR));
		}
	}
	else
		EckDbgBreak();
}





BOOL CPlayListFileWriter::Open(PCWSTR pszFile, PlType eType)
{
	if (m_File.Open(pszFile, eck::FCD_COVER, GENERIC_WRITE) == INVALID_HANDLE_VALUE)
		return FALSE;
	m_File.MoveToBegin();
	m_File += sizeof(LISTFILEHEADER_1);
	return TRUE;
}

void CPlayListFileWriter::AddOrgItem(const PLDATA& Item, const LISTFILE_STRINFO& StrInfo)
{
	++m_Header.cItem;
	LISTFILEITEM_2 Item2;
	Item2.cchName = (int)StrInfo.svName.size();
	Item2.cchFile = (int)StrInfo.svFile.size();
	Item2.cchTitle = (int)StrInfo.svTitle.size();
	Item2.cchArtist = (int)StrInfo.svArtist.size();
	Item2.cchAlbum = (int)StrInfo.svAlbum.size();
	Item2.cchGenre = (int)StrInfo.svGenre.size();
	Item2.s = Item;
	m_File << Item2;
	WriteStringView(StrInfo.svName);
	WriteStringView(StrInfo.svFile);
	WriteStringView(StrInfo.svTitle);
	WriteStringView(StrInfo.svArtist);
	WriteStringView(StrInfo.svAlbum);
	WriteStringView(StrInfo.svGenre);
}

void CPlayListFileWriter::SetRecentOptions()
{
}

void CPlayListFileWriter::SetViewOptions(const eck::CRefStrW& rsQuery)
{
}

void CPlayListFileWriter::SetStage(Stage eStage)
{
	m_eStage = eStage;
}

void CPlayListFileWriter::AddGroup(const eck::CRefStrW& rsName,
	_In_reads_(cOrg) const int* pidxOrg, size_t cOrg)
{
	LISTFILE_GROUP Group;
	Group.cchName = (int)rsName.Size();
	Group.cItem = (int)cOrg;
	m_File << Group;
	WriteStringView(rsName.ToStringView());
	m_File.Write(pidxOrg, DWORD(cOrg * sizeof(int)));
}

void CPlayListFileWriter::AddFlat(
	_In_reads_(cOrg) const int* pidxOrg, size_t cOrg)
{
	m_File.Write(pidxOrg, DWORD(cOrg * sizeof(int)));
}

void CPlayListFileWriter::BeginBookMark()
{
	m_Header.ocbBookMark = m_File.GetCurrPos();
	m_File += sizeof(BOOKMARKHEADER);
}

void CPlayListFileWriter::AddBookmark(const BOOKMARKITEM& Item, eck::CRefStrW& rsName)
{
	auto Item2{ Item };
	Item2.cchName = (int)rsName.Size();
	m_File << Item2;
	WriteStringView(rsName.ToStringView());
}

BOOL CPlayListFileWriter::End()
{
	auto dwCurr = m_File.GetCurrPos();
	m_File.MoveToBegin() << m_Header;
	m_File.MoveTo(dwCurr);
	m_File.End();
	m_File.Close();
	m_Header = { {'V','L','P','L'},VLLFVER_0 };
	return 0;
}