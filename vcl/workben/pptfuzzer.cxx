/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <tools/stream.hxx>
#include <vcl/FilterConfigItem.hxx>
#include "commonfuzzer.hxx"

#include <config_features.h>
#include <osl/detail/component-mapping.h>
#include <svx/svdobj.hxx>

extern "C" {
void * i18npool_component_getFactory( const char* , void* , void* );

void com_sun_star_i18n_LocaleDataImpl_get_implementation( void );
void com_sun_star_i18n_BreakIterator_Unicode_get_implementation( void );
void com_sun_star_i18n_BreakIterator_get_implementation( void );
void com_sun_star_comp_framework_Desktop_get_implementation( void );
void com_sun_star_i18n_CharacterClassification_Unicode_get_implementation( void );
void com_sun_star_i18n_CharacterClassification_get_implementation( void );
void com_sun_star_i18n_NativeNumberSupplier_get_implementation( void );
void com_sun_star_i18n_NumberFormatCodeMapper_get_implementation( void );
void com_sun_star_i18n_Transliteration_get_implementation( void );
void com_sun_star_drawing_EnhancedCustomShapeEngine_get_implementation( void );
void com_sun_star_drawing_SvxShapeCollection_get_implementation( void );
void SfxDocumentMetaData_get_implementation( void );
void com_sun_star_animations_AnimateColor_get_implementation( void );
void com_sun_star_animations_AnimateMotion_get_implementation( void );
void com_sun_star_animations_AnimateSet_get_implementation( void );
void com_sun_star_animations_AnimateTransform_get_implementation( void );
void com_sun_star_animations_Animate_get_implementation( void );
void com_sun_star_animations_Audio_get_implementation( void );
void com_sun_star_animations_Command_get_implementation( void );
void com_sun_star_animations_IterateContainer_get_implementation( void );
void com_sun_star_animations_ParallelTimeContainer_get_implementation( void );
void com_sun_star_animations_SequenceTimeContainer_get_implementation( void );
void com_sun_star_animations_TransitionFilter_get_implementation( void );
void com_sun_star_comp_comphelper_OPropertyBag( void );
void com_sun_star_comp_uui_UUIInteractionHandler_get_implementation( void );
void emfio_emfreader_XEmfParser_get_implementation( void );
void i18npool_CalendarImpl_get_implementation( void );
void i18npool_Calendar_gregorian_get_implementation( void );
void unoxml_CXPathAPI_get_implementation( void );
void unoxml_CDocumentBuilder_get_implementation( void );
void sd_PresentationDocument_get_implementation( void );
void ucb_UcbCommandEnvironment_get_implementation( void );
void ucb_UcbContentProviderProxyFactory_get_implementation( void );
void ucb_UcbPropertiesManager_get_implementation( void );
void ucb_UcbStore_get_implementation( void );
void ucb_UniversalContentBroker_get_implementation( void );
void ucb_OFileAccess_get_implementation( void );
}

const lib_to_factory_mapping *
lo_get_factory_map(void)
{
    static lib_to_factory_mapping map[] = {
        { "libi18npoollo.a", i18npool_component_getFactory },
        { 0, 0 }
    };

    return map;
}

const lib_to_constructor_mapping *
lo_get_constructor_map(void)
{
    static lib_to_constructor_mapping map[] = {
        { "com_sun_star_i18n_LocaleDataImpl_get_implementation", com_sun_star_i18n_LocaleDataImpl_get_implementation },
        { "com_sun_star_i18n_BreakIterator_Unicode_get_implementation", com_sun_star_i18n_BreakIterator_Unicode_get_implementation },
        { "com_sun_star_i18n_BreakIterator_get_implementation", com_sun_star_i18n_BreakIterator_get_implementation },
        { "com_sun_star_comp_framework_Desktop_get_implementation", com_sun_star_comp_framework_Desktop_get_implementation },
        { "com_sun_star_i18n_CharacterClassification_Unicode_get_implementation", com_sun_star_i18n_CharacterClassification_Unicode_get_implementation },
        { "com_sun_star_i18n_CharacterClassification_get_implementation", com_sun_star_i18n_CharacterClassification_get_implementation },
        { "com_sun_star_i18n_NativeNumberSupplier_get_implementation", com_sun_star_i18n_NativeNumberSupplier_get_implementation },
        { "com_sun_star_i18n_NumberFormatCodeMapper_get_implementation", com_sun_star_i18n_NumberFormatCodeMapper_get_implementation },
        { "com_sun_star_i18n_Transliteration_get_implementation", com_sun_star_i18n_Transliteration_get_implementation },
        { "com_sun_star_drawing_EnhancedCustomShapeEngine_get_implementation", com_sun_star_drawing_EnhancedCustomShapeEngine_get_implementation },
        { "com_sun_star_drawing_SvxShapeCollection_get_implementation", com_sun_star_drawing_SvxShapeCollection_get_implementation },
        { "SfxDocumentMetaData_get_implementation", SfxDocumentMetaData_get_implementation },
        { "com_sun_star_animations_AnimateColor_get_implementation", com_sun_star_animations_AnimateColor_get_implementation },
        { "com_sun_star_animations_AnimateMotion_get_implementation", com_sun_star_animations_AnimateMotion_get_implementation },
        { "com_sun_star_animations_AnimateSet_get_implementation", com_sun_star_animations_AnimateSet_get_implementation },
        { "com_sun_star_animations_AnimateTransform_get_implementation", com_sun_star_animations_AnimateTransform_get_implementation },
        { "com_sun_star_animations_Animate_get_implementation", com_sun_star_animations_Animate_get_implementation },
        { "com_sun_star_animations_Audio_get_implementation", com_sun_star_animations_Audio_get_implementation },
        { "com_sun_star_animations_Command_get_implementation", com_sun_star_animations_Command_get_implementation },
        { "com_sun_star_animations_IterateContainer_get_implementation", com_sun_star_animations_IterateContainer_get_implementation },
        { "com_sun_star_animations_ParallelTimeContainer_get_implementation", com_sun_star_animations_ParallelTimeContainer_get_implementation },
        { "com_sun_star_animations_SequenceTimeContainer_get_implementation", com_sun_star_animations_SequenceTimeContainer_get_implementation },
        { "com_sun_star_animations_TransitionFilter_get_implementation", com_sun_star_animations_TransitionFilter_get_implementation },
        { "com_sun_star_comp_comphelper_OPropertyBag", com_sun_star_comp_comphelper_OPropertyBag },
        { "com_sun_star_comp_uui_UUIInteractionHandler_get_implementation", com_sun_star_comp_uui_UUIInteractionHandler_get_implementation },
        { "emfio_emfreader_XEmfParser_get_implementation", emfio_emfreader_XEmfParser_get_implementation},
        { "i18npool_CalendarImpl_get_implementation", i18npool_CalendarImpl_get_implementation},
        { "i18npool_Calendar_gregorian_get_implementation", i18npool_Calendar_gregorian_get_implementation},
        { "unoxml_CXPathAPI_get_implementation", unoxml_CXPathAPI_get_implementation },
        { "unoxml_CDocumentBuilder_get_implementation", unoxml_CDocumentBuilder_get_implementation },
        { "sd_PresentationDocument_get_implementation", sd_PresentationDocument_get_implementation },
        { "ucb_UcbCommandEnvironment_get_implementation", ucb_UcbCommandEnvironment_get_implementation, },
        { "ucb_UcbContentProviderProxyFactory_get_implementation", ucb_UcbContentProviderProxyFactory_get_implementation },
        { "ucb_UcbPropertiesManager_get_implementation", ucb_UcbPropertiesManager_get_implementation },
        { "ucb_UcbStore_get_implementation", ucb_UcbStore_get_implementation },
        { "ucb_UniversalContentBroker_get_implementation", ucb_UniversalContentBroker_get_implementation },
        { "ucb_OFileAccess_get_implementation", ucb_OFileAccess_get_implementation },
        { 0, 0 }
    };

    return map;
}

extern "C" void* lo_get_custom_widget_func(const char*)
{
    return nullptr;
}

extern "C" void* SdCreateDialogFactory()
{
    return nullptr;
}

extern "C" bool TestImportPPT(SvStream &rStream);

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    if (__lsan_disable)
        __lsan_disable();

    CommonInitialize(argc, argv);
    SdrObject::GetGlobalDrawObjectItemPool();

    comphelper::getProcessServiceFactory()->createInstance("com.sun.star.comp.Draw.PresentationDocument");

    if (__lsan_enable)
        __lsan_enable();

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    SvMemoryStream aStream(const_cast<uint8_t*>(data), size, StreamMode::READ);
    (void)TestImportPPT(aStream);
    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
