/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#pragma once

#include <com/sun/star/animations/XAnimationNode.hpp>
#include <com/sun/star/animations/XTimeContainer.hpp>
#include <com/sun/star/animations/XAudio.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/util/XChangesListener.hpp>
#include <rtl/ref.hxx>
#include <vcl/timer.hxx>
#include <tools/long.hxx>
#include "sddllapi.h"
#include <list>
#include <vector>
#include <map>
#include <memory>

class SdrPathObj;
class SdrModel;

namespace sd {

enum class EValue { To, By };

class CustomAnimationEffect;

class CustomAnimationPreset;
typedef std::shared_ptr< CustomAnimationPreset > CustomAnimationPresetPtr;

typedef std::shared_ptr< CustomAnimationEffect > CustomAnimationEffectPtr;

typedef std::list< CustomAnimationEffectPtr > EffectSequence;

class EffectSequenceHelper;

class CustomAnimationEffect final
{
    friend class MainSequence;
    friend class EffectSequenceHelper;

public:
    CustomAnimationEffect( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    ~CustomAnimationEffect();

    const css::uno::Reference< css::animations::XAnimationNode >& getNode() const { return mxNode; }
    void setNode( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    void replaceNode( const css::uno::Reference< css::animations::XAnimationNode >& xNode );

    CustomAnimationEffectPtr clone() const;

    // attributes
    const OUString&    getPresetId() const { return maPresetId; }
    const OUString&    getPresetSubType() const { return maPresetSubType; }
    const OUString&    getProperty() const { return maProperty; }

    sal_Int16       getPresetClass() const { return mnPresetClass; }
    void            setPresetClassAndId( sal_Int16 nPresetClass, const OUString& rPresetId );

    sal_Int16       getNodeType() const { return mnNodeType; }
    void                           setNodeType( sal_Int16 nNodeType );

    css::uno::Any   getRepeatCount() const;
    void            setRepeatCount( const css::uno::Any& rRepeatCount );

    css::uno::Any   getEnd() const;
    void            setEnd( const css::uno::Any& rEnd );

    sal_Int16       getFill() const { return mnFill; }
    void            setFill( sal_Int16 nFill );

    double          getBegin() const { return mfBegin; }
    void                           setBegin( double fBegin );

    double          getDuration() const { return mfDuration; }
    void                           setDuration( double fDuration );

    double          getAbsoluteDuration() const { return mfAbsoluteDuration; }

    sal_Int16       getIterateType() const { return mnIterateType; }
    void                           setIterateType( sal_Int16 nIterateType );

    double          getIterateInterval() const { return mfIterateInterval; }
    void                           setIterateInterval( double fIterateInterval );

    const css::uno::Any& getTarget() const { return maTarget; }
    void                          setTarget( const css::uno::Any& rTarget );

    bool             hasAfterEffect() const { return mbHasAfterEffect; }
    void            setHasAfterEffect( bool bHasAfterEffect ) { mbHasAfterEffect = bHasAfterEffect; }

    const css::uno::Any& getDimColor() const { return maDimColor; }
    void            setDimColor( const css::uno::Any& rDimColor ) { maDimColor = rDimColor; }

    bool            IsAfterEffectOnNext() const { return mbAfterEffectOnNextEffect; }
    void            setAfterEffectOnNext( bool bOnNextEffect ) { mbAfterEffectOnNextEffect = bOnNextEffect; }

    sal_Int32       getParaDepth() const { return mnParaDepth; }

    bool            hasText() const { return mbHasText; }

    sal_Int16       getCommand() const { return mnCommand; }

    double          getAcceleration() const { return mfAcceleration; }
    void            setAcceleration( double fAcceleration );

    double          getDecelerate() const { return mfDecelerate; }
    void            setDecelerate( double fDecelerate );

    bool            getAutoReverse() const { return mbAutoReverse; }
    void            setAutoReverse( bool bAutoReverse );

    css::uno::Any  getProperty( sal_Int32 nNodeType, std::u16string_view rAttributeName, EValue eValue );
    bool           setProperty( sal_Int32 nNodeType, std::u16string_view rAttributeName, EValue eValue, const css::uno::Any& rValue );

    css::uno::Any  getTransformationProperty( sal_Int32 nTransformType, EValue eValue );
    bool           setTransformationProperty( sal_Int32 nTransformType, EValue eValue, const css::uno::Any& rValue );

    css::uno::Any  getColor( sal_Int32 nIndex );
    void            setColor( sal_Int32 nIndex, const css::uno::Any& rColor );

    sal_Int32       getGroupId() const { return mnGroupId; }
    void            setGroupId( sal_Int32 nGroupId );

    sal_Int16       getTargetSubItem() const { return mnTargetSubItem; }
    void                           setTargetSubItem( sal_Int16 nSubItem );

    OUString getPath() const;
    void setPath( const OUString& rPath );

    bool checkForText( const std::vector<sal_Int32>* paragraphNumberingLevel = nullptr );
    bool calculateIterateDuration();

    void setAudio( const css::uno::Reference< css::animations::XAudio >& xAudio );
    bool getStopAudio() const;
    void setStopAudio();
    void createAudio( const css::uno::Any& rSource );
    void removeAudio();
    const css::uno::Reference< css::animations::XAudio >& getAudio() const { return mxAudio; }

    EffectSequenceHelper*   getEffectSequence() const { return mpEffectSequence; }

    // helper
    /// @throws css::uno::Exception
    css::uno::Reference< css::animations::XAnimationNode > createAfterEffectNode() const;
    css::uno::Reference< css::drawing::XShape > getTargetShape() const;

    // static helpers
    static sal_Int32 get_node_type( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    static sal_Int32 getNumberOfSubitems( const css::uno::Any& aTarget, sal_Int16 nIterateType );

    rtl::Reference<SdrPathObj> createSdrPathObjFromPath(SdrModel& rTargetModel);
    void updateSdrPathObjFromPath( SdrPathObj& rPathObj );
    void updatePathFromSdrPathObj( const SdrPathObj& rPathObj );

private:
    void setEffectSequence( EffectSequenceHelper* pSequence ) { mpEffectSequence = pSequence; }

    sal_Int16       mnNodeType;
    OUString        maPresetId;
    OUString        maPresetSubType;
    OUString        maProperty;
    sal_Int16       mnPresetClass;
    sal_Int16       mnFill;
    double          mfBegin;
    double          mfDuration;                 // this is the maximum duration of the subeffects
    double          mfAbsoluteDuration;         // this is the maximum duration of the subeffects including possible iterations
    sal_Int32       mnGroupId;
    sal_Int16       mnIterateType;
    double          mfIterateInterval;
    sal_Int32       mnParaDepth;
    bool            mbHasText;
    double          mfAcceleration;
    double          mfDecelerate;
    bool            mbAutoReverse;
    sal_Int16       mnTargetSubItem;
    sal_Int16       mnCommand;

    EffectSequenceHelper* mpEffectSequence;

    css::uno::Reference< css::animations::XAnimationNode > mxNode;
    css::uno::Reference< css::animations::XAudio > mxAudio;
    css::uno::Any maTarget;

    bool        mbHasAfterEffect;
    css::uno::Any maDimColor;
    bool        mbAfterEffectOnNextEffect;
};

struct stl_CustomAnimationEffect_search_node_predict
{
    stl_CustomAnimationEffect_search_node_predict( const css::uno::Reference< css::animations::XAnimationNode >& xSearchNode );
    bool operator()( const CustomAnimationEffectPtr& pEffect ) const;
    const css::uno::Reference< css::animations::XAnimationNode >& mxSearchNode;
};

/** this listener is implemented by UI components to track changes in the animation core */
class ISequenceListener
{
public:
    virtual void notify_change() = 0;

protected:
    ~ISequenceListener() {}
};

/** this class keeps track of a group of animations that build up
    a text animation for a single shape */
class CustomAnimationTextGroup
{
    friend class EffectSequenceHelper;

public:
    CustomAnimationTextGroup( const css::uno::Reference< css::drawing::XShape >& rTarget, sal_Int32 nGroupId );

    void reset();
    void addEffect( CustomAnimationEffectPtr const & pEffect );

    const EffectSequence& getEffects() const { return maEffects; }

    /* -1: as single object, 0: all at once, n > 0: by n Th paragraph */
    sal_Int32 getTextGrouping() const { return mnTextGrouping; }

    bool getAnimateForm() const { return mbAnimateForm; }
    bool getTextReverse() const { return mbTextReverse; }
    double getTextGroupingAuto() const { return mfGroupingAuto; }

private:
    EffectSequence maEffects;
    css::uno::Reference< css::drawing::XShape > maTarget;

    enum { PARA_LEVELS = 5 };

    sal_Int32 mnTextGrouping;
    bool mbAnimateForm;
    bool mbTextReverse;
    double mfGroupingAuto;
    sal_Int32 mnLastPara;
    sal_Int8 mnDepthFlags[PARA_LEVELS];
    sal_Int32 mnGroupId;
};

typedef std::shared_ptr< CustomAnimationTextGroup > CustomAnimationTextGroupPtr;
typedef std::map< sal_Int32, CustomAnimationTextGroupPtr > CustomAnimationTextGroupMap;

class SD_DLLPUBLIC EffectSequenceHelper
{
friend class MainSequence;

public:
    SAL_DLLPRIVATE EffectSequenceHelper();
    SAL_DLLPRIVATE EffectSequenceHelper( css::uno::Reference< css::animations::XTimeContainer > xSequenceRoot );
    SAL_DLLPRIVATE virtual ~EffectSequenceHelper();

    SAL_DLLPRIVATE virtual css::uno::Reference< css::animations::XAnimationNode > getRootNode();

    SAL_DLLPRIVATE CustomAnimationEffectPtr append( const CustomAnimationPresetPtr& pDescriptor, const css::uno::Any& rTarget, double fDuration );
    SAL_DLLPRIVATE CustomAnimationEffectPtr append( const SdrPathObj& rPathObj, const css::uno::Any& rTarget, double fDuration, const OUString& rPresetId );
    void append( const CustomAnimationEffectPtr& pEffect );
    SAL_DLLPRIVATE void replace( const CustomAnimationEffectPtr& pEffect, const CustomAnimationPresetPtr& pDescriptor, double fDuration );
    SAL_DLLPRIVATE void replace( const CustomAnimationEffectPtr& pEffect, const CustomAnimationPresetPtr& pDescriptor, const OUString& rPresetSubType, double fDuration );
    SAL_DLLPRIVATE void remove( const CustomAnimationEffectPtr& pEffect );
    SAL_DLLPRIVATE void moveToBeforeEffect( const CustomAnimationEffectPtr& pEffect,  const CustomAnimationEffectPtr& pInsertBefore);

    SAL_DLLPRIVATE void create( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    SAL_DLLPRIVATE void createEffectsequence( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    SAL_DLLPRIVATE void processAfterEffect( const css::uno::Reference< css::animations::XAnimationNode >& xNode );
    SAL_DLLPRIVATE void createEffects( const css::uno::Reference< css::animations::XAnimationNode >& xNode );

    SAL_DLLPRIVATE sal_Int32 getCount() const { return sal::static_int_cast< sal_Int32 >( maEffects.size() ); }

    SAL_DLLPRIVATE virtual CustomAnimationEffectPtr findEffect( const css::uno::Reference< css::animations::XAnimationNode >& xNode ) const;

    SAL_DLLPRIVATE virtual bool disposeShape( const css::uno::Reference< css::drawing::XShape >& xShape );
    SAL_DLLPRIVATE virtual void insertTextRange( const css::uno::Any& aTarget );
    SAL_DLLPRIVATE virtual void disposeTextRange( const css::uno::Any& aTarget );
    SAL_DLLPRIVATE virtual bool hasEffect( const css::uno::Reference< css::drawing::XShape >& xShape );
    SAL_DLLPRIVATE virtual void onTextChanged( const css::uno::Reference< css::drawing::XShape >& xShape );

    /** this method rebuilds the animation nodes */
    SAL_DLLPRIVATE virtual void rebuild();

    SAL_DLLPRIVATE EffectSequence::iterator getBegin() { return maEffects.begin(); }
    SAL_DLLPRIVATE EffectSequence::iterator getEnd() { return maEffects.end(); }
    SAL_DLLPRIVATE EffectSequence::iterator find( const CustomAnimationEffectPtr& pEffect );

    SAL_DLLPRIVATE EffectSequence& getSequence() { return maEffects; }

    SAL_DLLPRIVATE void addListener( ISequenceListener* pListener );
    SAL_DLLPRIVATE void removeListener( ISequenceListener* pListener );

    // text group methods

    SAL_DLLPRIVATE CustomAnimationTextGroupPtr findGroup( sal_Int32 nGroupId );
    CustomAnimationTextGroupPtr createTextGroup(const CustomAnimationEffectPtr& pEffect,
                                                sal_Int32 nTextGrouping, double fTextGroupingAuto,
                                                bool bAnimateForm, bool bTextReverse);
    SAL_DLLPRIVATE void setTextGrouping( const CustomAnimationTextGroupPtr& pTextGroup, sal_Int32 nTextGrouping );
    SAL_DLLPRIVATE void setAnimateForm( const CustomAnimationTextGroupPtr& pTextGroup, bool bAnimateForm );
    SAL_DLLPRIVATE void setTextGroupingAuto( const CustomAnimationTextGroupPtr& pTextGroup, double fTextGroupingAuto );
    SAL_DLLPRIVATE void setTextReverse( const  CustomAnimationTextGroupPtr& pTextGroup, bool bAnimateForm );

    SAL_DLLPRIVATE sal_Int32 getSequenceType() const { return mnSequenceType; }

    SAL_DLLPRIVATE const css::uno::Reference< css::drawing::XShape >& getTriggerShape() const { return mxEventSource; }
    SAL_DLLPRIVATE void setTriggerShape( const css::uno::Reference< css::drawing::XShape >& xTrigger ) { mxEventSource = xTrigger; }

    SAL_DLLPRIVATE virtual sal_Int32 getOffsetFromEffect( const CustomAnimationEffectPtr& xEffect ) const;
    SAL_DLLPRIVATE virtual CustomAnimationEffectPtr getEffectFromOffset( sal_Int32 nOffset ) const;

protected:
    SAL_DLLPRIVATE virtual void implRebuild();
    SAL_DLLPRIVATE virtual void reset();

    SAL_DLLPRIVATE void createTextGroupParagraphEffects( const CustomAnimationTextGroupPtr& pTextGroup, const CustomAnimationEffectPtr& pEffect, bool bUsed );

    SAL_DLLPRIVATE void notify_listeners();

    SAL_DLLPRIVATE void updateTextGroups();

    SAL_DLLPRIVATE bool getParagraphNumberingLevels( const css::uno::Reference< css::drawing::XShape >& xShape, std::vector< sal_Int32 >& rParagraphNumberingLevel );

protected:
    css::uno::Reference< css::animations::XTimeContainer > mxSequenceRoot;
    EffectSequence maEffects;
    std::list< ISequenceListener* > maListeners;
    CustomAnimationTextGroupMap maGroupMap;
    sal_Int32 mnSequenceType;
    css::uno::Reference< css::drawing::XShape > mxEventSource;
};

class MainSequence;

class InteractiveSequence final : public EffectSequenceHelper
{
friend class MainSequence;
friend class MainSequenceChangeGuard;

public:
    InteractiveSequence( const css::uno::Reference< css::animations::XTimeContainer >& xSequenceRoot, MainSequence* pMainSequence );

    /** this method rebuilds the animation nodes */
    virtual void rebuild() override;

private:
    virtual void implRebuild() override;

    MainSequence*   mpMainSequence;
};

typedef std::shared_ptr< InteractiveSequence > InteractiveSequencePtr;
typedef std::vector< InteractiveSequencePtr > InteractiveSequenceVector;

class MainSequence final : public EffectSequenceHelper, public ISequenceListener
{
    friend class UndoAnimation;
    friend class MainSequenceRebuildGuard;
    friend class MainSequenceChangeGuard;

public:
    MainSequence();
    MainSequence( const css::uno::Reference< css::animations::XAnimationNode >& xTimingRootNode );
    virtual ~MainSequence() override;

    virtual css::uno::Reference< css::animations::XAnimationNode > getRootNode() override;
    void reset( const css::uno::Reference< css::animations::XAnimationNode >& xTimingRootNode );

    /** this method rebuilds the animation nodes */
    virtual void rebuild() override;

    virtual CustomAnimationEffectPtr findEffect( const css::uno::Reference< css::animations::XAnimationNode >& xNode ) const override;

    virtual bool disposeShape( const css::uno::Reference< css::drawing::XShape >& xShape ) override;
    virtual void insertTextRange( const css::uno::Any& aTarget ) override;
    virtual void disposeTextRange( const css::uno::Any& aTarget ) override;
    virtual bool hasEffect( const css::uno::Reference< css::drawing::XShape >& xShape ) override;
    virtual void onTextChanged( const css::uno::Reference< css::drawing::XShape >& xShape ) override;

    const InteractiveSequenceVector& getInteractiveSequenceVector() const { return maInteractiveSequenceVector; }

    virtual void notify_change() override;

    bool setTrigger( const CustomAnimationEffectPtr& pEffect, const css::uno::Reference< css::drawing::XShape >& xTriggerShape );

    /** starts a timer that recreates the internal structure from the API core after 1 second */
    void startRecreateTimer();

    /** starts a timer that rebuilds the API core from the internal structure after 1 second */
    void startRebuildTimer();

    virtual sal_Int32 getOffsetFromEffect( const CustomAnimationEffectPtr& xEffect ) const override;
    virtual CustomAnimationEffectPtr getEffectFromOffset( sal_Int32 nOffset ) const override;

private:
    /** permits rebuilds until unlockRebuilds() is called. All rebuild calls during a locked sequence are
        process after unlockRebuilds() call. lockRebuilds() and unlockRebuilds() calls can be nested. */
    void lockRebuilds();
    void unlockRebuilds();

    DECL_LINK(onTimerHdl, Timer *, void);

    virtual void implRebuild() override;

    void init();

    void createMainSequence();
    virtual void reset() override;

    InteractiveSequencePtr createInteractiveSequence( const css::uno::Reference< css::drawing::XShape >& xShape );

    InteractiveSequenceVector maInteractiveSequenceVector;

    css::uno::Reference< css::util::XChangesListener > mxChangesListener;
    css::uno::Reference< css::animations::XTimeContainer > mxTimingRootNode;
    Timer maTimer;
    bool mbTimerMode;
    bool mbRebuilding;

    ::tools::Long mnRebuildLockGuard;
    bool mbPendingRebuildRequest;
    sal_Int32 mbIgnoreChanges;
};

typedef std::shared_ptr< MainSequence > MainSequencePtr;

class MainSequenceRebuildGuard
{
public:
    MainSequenceRebuildGuard( MainSequencePtr pMainSequence );
    ~MainSequenceRebuildGuard();

private:
    MainSequencePtr mpMainSequence;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
