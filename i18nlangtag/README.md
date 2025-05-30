# Language Tags

Code for language tags, LanguageTag wrapper for liblangtag and converter between BCP47 language tags, Locale(Language,Country,Variant) and MS-LangIDs.

Basic functionality used by almost every other module including comphelper, so even don't use that string helpers in this code to not create circular dependencies. Stick with sal and rtl!



If Microsoft introduced a new LCID for a locale that we previously defined as `LANGUAGE_USER_...`, for example `LANGUAGE_CATALAN_VALENCIAN` that we had as `LANGUAGE_USER_CATALAN_VALENCIAN`:

* `include/i18nlangtag/lang.h`
    * add the new `LANGUAGE_...` value as defined by MS, here `LANGUAGE_CATALAN_VALENCIAN`
    * rename the `LANGUAGE_USER_...` definition to `LANGUAGE_OBSOLETE_USER_...`, here `LANGUAGE_USER_CATALAN_VALENCIAN` to `LANGUAGE_OBSOLETE_USER_CATALAN_VALENCIAN`
    * add a `#define LANGUAGE_USER_CATALAN_VALENCIAN LANGUAGE_CATALAN_VALENCIAN`
        * so `svtools/source/misc/langtab.src` (where the defined name is an identifier) and other places using `LANGUAGE_USER_CATALAN_VALENCIAN` do not need to be changed

* `i18nlangtag/source/isolang/isolang.cxx`
    * insert a mapping with `LANGUAGE_CATALAN_VALENCIAN` before (!) the existing `LANGUAGE_USER_CATALAN_VALENCIAN`
    * rename the `LANGUAGE_USER_CATALAN_VALENCIAN` to `LANGUAGE_OBSOLETE_USER_CATALAN_VALENCIAN`
        * so converting the tag maps to the new `LANGUAGE_CATALAN_VALENCIAN` and converting the old `LANGUAGE_OBSOLETE_USER_CATALAN_VALENCIAN` still maps to the tag.

* `i18nlangtag/source/isolang/mslangid.cxx`
    * add an entry to `MsLangId::getReplacementForObsoleteLanguage()` to convert `LANGUAGE_OBSOLETE_USER_CATALAN_VALENCIAN` to `LANGUAGE_CATALAN_VALENCIAN`


When changing a (translation's) language tag (for example, `ca-XV` to `ca-valencia` or `sh` to `sr-Latn`):

* `solenv/inc/langlist.mk`
    * replace the tag and sort alphabetically

* in `translations/source` do  `git mv old-tag new-tag`
    * note that translations is a git submodule so <https://wiki.SnipeOffice.org/Development/Submodules applies>

* `i18nlangtag/source/isolang/isolang.cxx`
    * maintain the old tag's mapping entry in `aImplIsoLangEntries` to be able to read existing documents using that code
    * add the new tag's mapping to `aImplBcp47CountryEntries` or `aImplIsoLangScriptEntries`
    * change `mnOverride` from 0 to `kSAME` in `aImplIsoLangScriptEntries` or `aImplIsoLangEntries`

* `i18nlangtag/source/languagetag/languagetag.cxx`
    * add the new tag's fallback strings to the fallback of the old tag in `LanguageTag::getFallbackStrings()`

* `i18nlangtag/qa/cppunit/test_languagetag.cxx`
    * add a unit test for the new tag and old tag

* `l10ntools/source/ulfconv/msi-encodinglist.txt`
    * replace the tag and sort alphabetically

* `setup_native/source/packinfo/spellchecker_selection.txt`
    * replace the tag and sort alphabetically

If locale data exists:

* `i18npool/source/localedata/data/*.xml` for example `i18npool/source/localedata/data/sh_RS.xml`
    * in the `<LC_INFO>` element
        * change `<LangID>` to `qlt`
        * after the `<Country>` element add a `<Variant>` element with the new full BCP 47 tag, for example `sr-Latn-RS`
            * note that `<Variant>` has no `<VariantID>` or `<DefaultName>` child elements
    * if any of the other `*.xml` files reference the locale in a `ref="..."` attribute, change those too; note that these references use '`_`' underscore instead of '`-`' hyphen just like the file names do
    * rename `sh_RS.xml` to `sr_Latn_RS.xml`, `git mv sh_RS.xml sr_Latn_RS.xml`

* `i18npool/source/localedata/localedata.cxx`
    * in `aLibTable` change the entry from old `sh_RS` to new `sr_Latn_RS`, do not sort the table

* `i18npool/Library_localedata_*.mk`     for example `i18npool/Library_localedata_euro.mk`
    * change the entry for the changed `.xml` file, for example `CustomTarget/i18npool/localedata/localedata_sh_RS` to `CustomTarget/i18npool/localedata/localedata_sr_Latn_RS`, sort the list alphabetically

If dictionary exists:

* `dictionaries/*/dictionaries.xcu`      for example `dictionaries/sr/dictionaries.xcu`
    * change the affected `<node oor:name="...">` elements to something corresponding, for example `<node oor:name="HunSpellDic_sh" ...>` to `<node oor:name="HunSpellDic_sr_Latn" ...>`
    * in the `Locales` properties change the `<value>` element, for example `<value>sh-RS</value>` to `<value>sr-Latn-RS</value>`

If dictionary is to be renamed, for example `ku-TR` to `kmr-Latn`:

* `dictionaries/*/*`                     for example `dictionaries/ku_TR/*`
    * if appropriate rename `*.dic` and `*.aff` files, for example `ku_TR.dic` to `kmr_Latn.dic` and `ku_TR.aff` to `kmr_Latn.aff`
* `dictionaries/Dictionary_*.mk`         for example `dictionaries/Dictionary_ku_TR.mk`
    * rename file, for example to `Dictionary_kmr_Latn.mk`
    * change all locale dependent file names and target, for example `ku_TR` to `kmr_Latn` AND `ku-TR` to `kmr-Latn`; note '`-`' and '`_`' separators, both are used!
* `dictionaries/Module_dictionaries.mk`
    * change `Dictionary_*` (`Dictionary_ku-TR` to `Dictionary_kmr-Latn`) and sort alphabetically
* `scp2/source/ooo/common_brand.scp`
    * `DosName = "dict-ku-TR";`
        * change to `"dict-kmr-Latn"`
* `scp2/source/ooo/file_ooo.scp`
    * File `gid_File_Extension_Dictionary_Ku_Tr`
        * change to `gid_File_Extension_Dictionary_Kmr_Latn`
    * `Name = "Dictionary/dict-ku-TR.filelist";`
        * change to `"Dictionary/dict-kmr-Latn.filelist"`
* `scp2/source/ooo/module_ooo.scp`
    * Module `gid_Module_Root_Extension_Dictionary_Ku_Tr`
        * change to `gid_Module_Root_Extension_Dictionary_Kmr_Latn`
    * `MOD_NAME_DESC` ( `MODULE_EXTENSION_DICTIONARY_KU_TR` );
        * change to `MODULE_EXTENSION_DICTIONARY_KMR_LATN`
    * `Files = (gid_File_Extension_Dictionary_Ku_Tr);`
        * change to `gid_File_Extension_Dictionary_Kmr_Latn`
    * `Spellcheckerlanguage = "ku-TR";`
        * change to `"kmr-Latn"`
* `scp2/source/ooo/module_ooo.ulf`
    * [`STR_NAME_MODULE_EXTENSION_DICTIONARY_KU_TR`]
        * change to `STR_NAME_MODULE_EXTENSION_DICTIONARY_KMR_LATN`
    * `en-US = "Kurdish (Turkey)"`
        * change to `"Kurdish, Northern, Latin script"`
    * [`STR_DESC_MODULE_EXTENSION_DICTIONARY_KU_TR`]
        * change to `STR_DESC_MODULE_EXTENSION_DICTIONARY_KMR_LATN`
    * `en-US = "Kurdish (Turkey)` spelling dictionary"
        * change to `"Kurdish, Northern, Latin script spelling dictionary"`
* `setup_native/source/packinfo/packinfo_office.txt`
    * `module = "gid_Module_Root_Extension_Dictionary_Ku_Tr"`
        * change to `"gid_Module_Root_Extension_Dictionary_Kmr_Latn"`
    * `solarispackagename = "%PACKAGEPREFIX%WITHOUTDOTUNIXPRODUCTNAME%BRANDPACKAGEVERSION-dict-ku-TR"`
        * change to `"...-dict-kmr-Latn"`
    * `packagename = "%UNIXPRODUCTNAME%BRANDPACKAGEVERSION-dict-ku-TR"`
        * change to `"...-dict-kmr-Latn"`
    * `description = "Ku-TR dictionary for %PRODUCTNAME %PRODUCTVERSION"`
        * change to `"Kmr-Latn dictionary ..."`

If `extras` exist, for example `extras/source/autotext/*`:

* `extras/Package_autocorr.mk`
    * replace `acor_*` entry, for example `acor_sh-RS.dat` to `acor_sr-Latn-RS.dat`, sort alphabetically

* `extras/CustomTarget_autocorr.mk`
    * in `extras_AUTOCORR_LANGS change` map entry, for example `sh-RS:sh-RS` to `sr-Latn-RS:sr-Latn-Rs`
    * in `extras_AUTOCORR_XMLFILES` change directory entries, for example `sh-RS/acor/DocumentList.xml` to `sr-Latn-RS/acor/DocumentList.xml`

* rename files accordingly, for example in `extras/source/autotext/lang/`  `git mv sh-RS sr-Latn-RS`

If `helpcontent` exists:

* `helpcontent2/source/auxiliary/*/*`       for example `helpcontent2/source/auxiliary/sh/*`
    * change `Language=...`, for example `Language=sh` to `Language=sr-Latn` in `helpcontent2/source/auxiliary/sh/*.cfg`
    * rename `helpcontent2/source/auxiliary/sh/`  `git mv sh sr-Latn`

For language packs:

* `scp2/source/ooo/module_langpack.ulf`
* `scp2/source/accessories/module_templates_accessories.ulf`
* `scp2/source/accessories/module_samples_accessories.ulf`
* `scp2/source/extensions/module_extensions_sun_templates.ulf`

    * If the upper-cased tag appears in any of these, replace it, for example `STR_NAME_MODULE_LANGPACK_SH` to `STR_NAME_MODULE_LANGPACK_SR_LATN`

