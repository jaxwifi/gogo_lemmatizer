// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Lemmatizer (www.aot.ru)
// ==========  Copyright by Alexey Sokirko, Andrey Putrin

#include "StdMorph.h"
#include "Lemmatizers.h"
#include "Paradigm.h"
#include "Paradigm.h"

#ifdef DETECT_MEMORY_LEAK
	#ifdef _DEBUG
	#define new DEBUG_NEW

	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

CFormInfo::CFormInfo()
{
	m_pParent = NULL;
	m_bFound = true;
	m_InnerAnnot.m_LemmaInfoNo = -1;
	
}

void	CFormInfo::Copy(const CFormInfo& from)
{
	m_InnerAnnot = from.m_InnerAnnot;
	m_pParent = from.m_pParent;
	m_InputWordBase = from.m_InputWordBase;
	m_bFound = from.m_bFound;
	m_bCutFlexia = from.m_bCutFlexia;
	m_bCutPrefixes = from.m_bCutPrefixes;
};


size_t  CFormInfo::GetLemmaPrefixLength() const 
{
	assert (IsValid());
	if (!IsValid()) return 0;
	if (!m_bFound) return 0;
	return m_pParent->m_Prefixes[m_InnerAnnot.m_PrefixNo].length();
};

void	CFormInfo::Create(const CLemmatizer*	pParent, const CAutomAnnotationInner& A, const string& InputWordForm, bool bFound)
{
	m_InnerAnnot = A;
	m_pParent = pParent;
	m_bFound = bFound;
	m_InputWordBase = InputWordForm;
	const CMorphForm& M = GetFlexiaModel().m_Flexia[A.m_ItemNo];
	size_t FlexLength = M.m_FlexiaStr.length();

	//  It can be so( if CLemmatizer::PredictByDataBase was used) that 
	//  the flexion  is not suffix of m_InputWordBase, but only part of  it.
	//  If so, then we cannot generate paradigm, since the current form cannot be 
	//  divided into two parts: the base and a known flexion.

	if (	m_bFound
		||	(		(	m_InputWordBase.length()>= FlexLength)
				&&	(m_InputWordBase.substr(m_InputWordBase.length()-FlexLength) == M.m_FlexiaStr)
			)
		)
	{
		m_bCutFlexia = true;
		m_InputWordBase.erase(m_InputWordBase.length() - M.m_FlexiaStr.length());
	}
	else
		m_bCutFlexia = false;

		

	const string& LemmPrefix = m_pParent->m_Prefixes[m_InnerAnnot.m_PrefixNo];
	if	(		m_bFound
			||	(		(m_InputWordBase.substr(0, LemmPrefix.length()) == LemmPrefix)
					&&	(m_InputWordBase.substr(LemmPrefix.length(), M.m_PrefixStr.length()) == M.m_PrefixStr)
				)
		)
	{
		m_InputWordBase.erase(0, LemmPrefix.length()+ M.m_PrefixStr.length());
		m_bCutPrefixes = true;
	}
	else
		m_bCutPrefixes = false;

	
};


const CLemmaInfoAndLemma& CFormInfo::GetLemmaInfo() const
{
	return m_pParent->m_LemmaInfos[m_InnerAnnot.m_LemmaInfoNo];
};

const CFlexiaModel& CFormInfo::GetFlexiaModel() const
{
	const CLemmaInfoAndLemma I = GetLemmaInfo();
	return m_pParent->m_FlexiaModels[I.m_LemmaInfo.m_FlexiaModelNo];
};

DWORD CFormInfo::GetParadigmId() const 
{
	assert (IsValid());
	if (!IsValid()) return -1;

	if (!m_bFound)
		return ErrorParadigmId;
	else
		return m_InnerAnnot.GetParadigmId();
};


string CFormInfo::GetCommonAncode() const 
{
	assert (IsValid());
	if (!IsValid()) return "";
	return GetLemmaInfo().m_LemmaInfo.GetCommonAncode();
};

string CFormInfo::GetLemSign() const 
{
	assert (IsValid());
	if (!IsValid()) return "";
	string Result  = m_bFound ? "+" : "-";
	string CommonAncode = GetCommonAncode();
	Result  += CommonAncode.empty() ? "??" : CommonAncode;
	return Result;
};



bool  CFormInfo::SetParadigmId(DWORD newVal) 
{
	assert (m_pParent);
	{
		CAutomAnnotationInner A;
		A.SplitParadigmId(newVal);
		if (A.m_LemmaInfoNo > m_pParent->m_LemmaInfos.size()) 
			return false;
		if (A.m_PrefixNo > m_pParent->m_Prefixes.size()) 
			return false;
		A.m_ItemNo = 0;
		A.m_nWeight = m_pParent->GetStatistic().get_HomoWeight(A.GetParadigmId(), 0);
		A.m_ModelNo = m_pParent->m_LemmaInfos[A.m_LemmaInfoNo].m_LemmaInfo.m_FlexiaModelNo;
		m_InnerAnnot = A;
	};

	m_bCutPrefixes = true;
	m_bCutFlexia = true;
	m_bFound = true;
	m_InputWordBase = GetSrcNorm();
	m_InputWordBase.erase(m_InputWordBase.length() - GetFlexiaModel().get_first_flex().length());

	return true;
}

DWORD  CFormInfo::GetCount () const 
{
	assert (IsValid());
	if (!IsValid()) return 0;
	return GetFlexiaModel().m_Flexia.size();
}


string CFormInfo::GetSrcNorm() const 
{
	assert (IsValid());
	if (!IsValid()) return "";

	//string result = m_pParent->m_Prefixes[m_InnerAnnot.m_PrefixNo];
	//result += m_pParent->m_Bases[GetLemmaInfo().m_LemmaStrNo].GetString();
	string result = m_pParent->m_Bases[GetLemmaInfo().m_LemmaStrNo].GetString();
	result += GetFlexiaModel().get_first_flex();
	return result;
}

int CFormInfo::GetHomonymWeightWithForm(WORD pos) const 
{
	assert (IsValid());
	if (!IsValid()) return 0;
	return  m_pParent->GetStatistic().get_HomoWeight(GetParadigmId(), pos);
}

int  CFormInfo::GetWordWeight() const
{
	assert (IsValid());
	if (!IsValid()) return 0;

	return m_pParent->GetStatistic().get_WordWeight(GetParadigmId());
}

void	CFormInfo::AttachLemmatizer(const CLemmatizer* parent)
{
	m_pParent = parent;
}

bool  CFormInfo::IsValid() const
{
	return		m_pParent 
			&&	(m_InnerAnnot.m_LemmaInfoNo != -1);
};

string  CFormInfo::GetSrcAncode() const
{
	assert (IsValid());
	if (!IsValid()) return "";

	const CLemmaInfoAndLemma& I = GetLemmaInfo();

	return GetFlexiaModel().m_Flexia[m_InnerAnnot.m_ItemNo].m_Gramcode;
}

string CFormInfo::GetAncode(WORD pos) const 
{
	assert (IsValid());
	if (!IsValid()) return "";

	return GetFlexiaModel().m_Flexia[pos].m_Gramcode.substr(0,2);
}



string CFormInfo::GetWordForm (WORD pos) const 
{
	assert (IsValid());
	if (!IsValid()) return "";

	const CMorphForm& F = GetFlexiaModel().m_Flexia[pos];
	
	string Result = m_InputWordBase;

	if (m_bCutFlexia)
		Result += F.m_FlexiaStr;

	if (m_bCutPrefixes)
		Result = m_pParent->m_Prefixes[m_InnerAnnot.m_PrefixNo]+F.m_PrefixStr + Result;

	return Result;
}


int  CFormInfo::GetHomonymWeight() const
{	
	assert (IsValid());
	if (!IsValid()) return 0;
	

	return m_InnerAnnot.m_nWeight;
}

BYTE	CFormInfo::GetAccentedVowel(WORD pos) const
{
	if (!m_bFound) return UnknownAccent;
	assert (IsValid());
	if (!IsValid()) return UnknownAccent;
	

	const CLemmaInfoAndLemma& I = GetLemmaInfo();

	if (I.m_LemmaInfo.m_AccentModelNo == UnknownAccentModelNo)
		return UnknownAccent;

	BYTE BackVowelNo = m_pParent->m_AccentModels[I.m_LemmaInfo.m_AccentModelNo].m_Accents[pos];
	string s = GetWordForm(pos);	
	RmlMakeLower(s, m_pParent->GetLanguage());
	return TransferReverseVowelNoToCharNo(s, BackVowelNo, m_pParent->GetLanguage());
};

BYTE	CFormInfo::GetSrcAccentedVowel() const
{
	return GetAccentedVowel(m_InnerAnnot.m_ItemNo);
};
