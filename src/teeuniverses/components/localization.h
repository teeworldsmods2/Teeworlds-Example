#ifndef __SHARED_LOCALIZATION__
#define __SHARED_LOCALIZATION__

/* BEGIN EDIT *********************************************************/
#include <teeuniverses/tl/hashtable.h>
#define CStorage IStorage
/* END EDIT ***********************************************************/

#include <unicode/ucnv.h>
#include <unicode/numfmt.h>
#include <unicode/upluralrules.h>
#include <unicode/tmutfmt.h>

#include <stdarg.h>

struct CLocalizableString
{
	const char* m_pText;
	
	CLocalizableString(const char* pText) :
		m_pText(pText)
	{ }
};

/* BEGIN EDIT *********************************************************/
#define _(TEXT) TEXT
#define _P(TEXT_SINGULAR, TEXT_PLURAL) TEXT_PLURAL
/* END EDIT ***********************************************************/

/* BEGIN EDIT *********************************************************/
class CLocalization
{
private:
	class CStorage* m_pStorage;
	inline class CStorage* Storage() { return m_pStorage; }
/* END EDIT ***********************************************************/
public:
	enum
	{
		PLURALTYPE_NONE=0,
		PLURALTYPE_ZERO,
		PLURALTYPE_ONE,
		PLURALTYPE_TWO,
		PLURALTYPE_FEW,
		PLURALTYPE_MANY,
		PLURALTYPE_OTHER,
		NUM_PLURALTYPES,
	};

	class IListener
	{
	public:
		virtual void OnLocalizationModified() = 0;
	};

	static const char *LanguageCodeByCountryCode(int country);
	static const char *FallbackLanguageForIpCountryCode(int Country);

	class CLanguage
	{
	protected:
		class CEntry
		{
		public:
			char* m_apVersions[NUM_PLURALTYPES];
			
			CEntry()
			{
				for(int i=0; i<NUM_PLURALTYPES; i++)
					m_apVersions[i] = NULL;
			}
			
			void Free()
			{
				for(int i=0; i<NUM_PLURALTYPES; i++)
					if(m_apVersions[i])
						delete[] m_apVersions[i];
			}
		};
		
	protected:
		char m_aName[64];
		char m_aFilename[64];
		char m_aParentFilename[64];
		bool m_Loaded;
		int m_Direction;
		
		hashtable< CEntry, 128 > m_Translations;
	
	public:
		UPluralRules* m_pPluralRules;
		UNumberFormat* m_pNumberFormater;
		UNumberFormat* m_pPercentFormater;
		icu::TimeUnitFormat* m_pTimeUnitFormater;
		
	public:
		CLanguage();
		CLanguage(const char* pName, const char* pFilename, const char* pParentFilename);
		~CLanguage();
		
		inline const char* GetParentFilename() const { return m_aParentFilename; }
		inline const char* GetFilename() const { return m_aFilename; }
		inline const char* GetName() const { return m_aName; }
		inline int GetWritingDirection() const { return m_Direction; }
		inline void SetWritingDirection(int Direction) { m_Direction = Direction; }
		inline bool IsLoaded() const { return m_Loaded; }
		bool Load(CLocalization* pLocalization, class CStorage* pStorage);
		const char* Localize(const char* pKey) const;
		const char* Localize_P(int Number, const char* pText) const;
	};
	
	enum
	{
		DIRECTION_LTR=0,
		DIRECTION_RTL,
		NUM_DIRECTIONS,
	};

protected:
	CLanguage* m_pMainLanguage;
	array<IListener*> m_pListeners;
	bool m_UpdateListeners;
	
	UConverter* m_pUtf8Converter;

public:
	array<CLanguage*> m_pLanguages;
	fixed_string128 m_Cfg_MainLanguage;

protected:
	const char* LocalizeWithDepth(const char* pLanguageCode, const char* pText, int Depth);
	const char* LocalizeWithDepth_P(const char* pLanguageCode, int Number, const char* pText, int Depth);
	
	void AppendNumber(dynamic_string& Buffer, int& BufferIter, CLanguage* pLanguage, int Number);
	void AppendPercent(dynamic_string& Buffer, int& BufferIter, CLanguage* pLanguage, double Number);
	void AppendDuration(dynamic_string& Buffer, int& BufferIter, CLanguage* pLanguage, int Number, icu::TimeUnit::UTimeUnitFields Type);

public:
/* BEGIN EDIT *********************************************************/
	CLocalization(class CStorage* pStorage);
/* END EDIT ***********************************************************/
	virtual ~CLocalization();
	
	virtual bool InitConfig(int argc, const char** argv);
/* BEGIN EDIT *********************************************************/
/* END EDIT ***********************************************************/
	virtual bool Init();
	virtual bool PreUpdate();
	
	void AddListener(IListener* pListener);
	void RemoveListener(IListener* pListener);
	
	inline bool GetWritingDirection() const { return (!m_pMainLanguage ? DIRECTION_LTR : m_pMainLanguage->GetWritingDirection()); }
	
	//localize
	const char* Localize(const char* pLanguageCode, const char* pText);
	//localize and find the appropriate plural form based on Number
	const char* Localize_P(const char* pLanguageCode, int Number, const char* pText);
	
	//format
	void Format_V(dynamic_string& Buffer, const char* pLanguageCode, const char* pText, va_list VarArgs);
	void Format(dynamic_string& Buffer, const char* pLanguageCode, const char* pText, ...);
	//localize, format
	void Format_VL(dynamic_string& Buffer, const char* pLanguageCode, const char* pText, va_list VarArgs);
	void Format_L(dynamic_string& Buffer, const char* pLanguageCode, const char* pText, ...);
	//localize, find the appropriate plural form based on Number and format
	void Format_VLP(dynamic_string& Buffer, const char* pLanguageCode, int Number, const char* pText, va_list VarArgs);
	void Format_LP(dynamic_string& Buffer, const char* pLanguageCode, int Number, const char* pText, ...);
	
	void ArabicShaping(dynamic_string& Buffer, int BufferStart = 0);
};

#endif
