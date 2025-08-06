#include "pch.h"
#include "Utils.h"

Tag::Result VltGetMusicInfo(PCWSTR pszFile,
	Tag::MUSICINFO& mi, const Tag::SIMPLE_OPT& Opt)
{
	Tag::CMediaFile mf{ pszFile };
	if (!mf.IsValid())
		return Tag::Result::FileAccessDenied;
	mi.Clear();
	if (mf.GetTagType() & Tag::TAG_FLAC)
	{
		Tag::CFlac x{ mf };
		x.ReadTag(0);
		x.SimpleGetSet(mi, Opt);
		mi.uMask &= ~mi.uMaskChecked;
	}
	if (mf.GetTagType() & (Tag::TAG_ID3V2_3 | Tag::TAG_ID3V2_4))
	{
		Tag::CID3v2 x{ mf };
		x.ReadTag(0);
		x.SimpleGetSet(mi, Opt);
		mi.uMask &= ~mi.uMaskChecked;
	}
	if (mf.GetTagType() & Tag::TAG_APE)
	{
		Tag::CApe x{ mf };
		x.ReadTag(0);
		x.SimpleGetSet(mi, Opt);
		mi.uMask &= ~mi.uMaskChecked;
	}
	if (mi.rsTitle.IsEmpty())
		mi.rsTitle.DupString(EckStrAndLen(L"未知标题"));
	if (mi.slArtist.Str.IsEmpty())
		mi.slArtist.PushBackString(L"未知艺术家"sv, {});
	if (mi.rsAlbum.IsEmpty())
		mi.rsAlbum.DupString(EckStrAndLen(L"未知专辑"));
	return Tag::Result::Ok;
}
