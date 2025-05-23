#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#
from ..ui.UIConsts import UIConsts
from ..ui.WizardDialog import PropertyNames, WizardDialog, uno
from .FaxWizardDialogConst import HIDMAIN, FaxWizardDialogConst
from .FaxWizardDialogResources import FaxWizardDialogResources


class FaxWizardDialog(WizardDialog):

    def __init__(self, xmsf):
        super(FaxWizardDialog,self).__init__(xmsf, HIDMAIN )

        #Load Resources
        self.resources = FaxWizardDialogResources()

        #set dialog properties...
        self.setDialogProperties(True, 210, True, 104, 52, 1, 1,
            self.resources.resFaxWizardDialog_title, 310)

        self.fontDescriptor4 = \
            uno.createUnoStruct('com.sun.star.awt.FontDescriptor')
        self.fontDescriptor5 = \
            uno.createUnoStruct('com.sun.star.awt.FontDescriptor')
        self.fontDescriptor4.Weight = 100
        self.fontDescriptor5.Weight = 150

    def buildStep1(self):
        self.optBusinessFax = self.insertRadioButton("optBusinessFax",
            FaxWizardDialogConst.OPTBUSINESSFAX_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTBUSINESSFAX_HID,
                self.resources.resoptBusinessFax_value, 97, 28, 1, 1, 184),
            self)
        self.lstBusinessStyle = self.insertListBox("lstBusinessStyle",
            FaxWizardDialogConst.LSTBUSINESSSTYLE_ACTION_PERFORMED,
            FaxWizardDialogConst.LSTBUSINESSSTYLE_ITEM_CHANGED,
            ("Dropdown", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (True, 12, FaxWizardDialogConst.LSTBUSINESSSTYLE_HID,
                180, 40, 1, 3, 74), self)
        self.optPrivateFax = self.insertRadioButton("optPrivateFax",
            FaxWizardDialogConst.OPTPRIVATEFAX_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTPRIVATEFAX_HID,
                self.resources.resoptPrivateFax_value,97, 81, 1, 2, 184), self)
        self.lstPrivateStyle = self.insertListBox("lstPrivateStyle",
            FaxWizardDialogConst.LSTPRIVATESTYLE_ACTION_PERFORMED,
            FaxWizardDialogConst.LSTPRIVATESTYLE_ITEM_CHANGED,
            ("Dropdown", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (True, 12, FaxWizardDialogConst.LSTPRIVATESTYLE_HID,
                180, 95, 1, 4, 74), self)
        self.insertLabel("lblBusinessStyle",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblBusinessStyle_value,
                110, 42, 1, 32, 60))

        self.insertLabel("lblTitle1",
            ("FontDescriptor", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor5, 16, self.resources.reslblTitle1_value,
                True, 91, 8, 1, 37, 212))
        self.insertLabel("lblPrivateStyle",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblPrivateStyle_value, 110, 95, 1, 50, 60))
        self.insertLabel("lblIntroduction",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (39, self.resources.reslblIntroduction_value,
                True, 104, 145, 1, 55, 199))
        self.ImageControl3 = self.insertInfoImage(92, 145, 1)

    def buildStep2(self):
        self.chkUseLogo = self.insertCheckBox("chkUseLogo",
            FaxWizardDialogConst.CHKUSELOGO_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSELOGO_HID,
                self.resources.reschkUseLogo_value, 97, 28, 0, 2, 5, 212),
            self)
        self.chkUseDate = self.insertCheckBox("chkUseDate",
            FaxWizardDialogConst.CHKUSEDATE_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSEDATE_HID,
                self.resources.reschkUseDate_value, 97, 43, 0, 2, 6, 212),
            self)
        self.chkUseCommunicationType = self.insertCheckBox(
            "chkUseCommunicationType",
            FaxWizardDialogConst.CHKUSECOMMUNICATIONTYPE_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSECOMMUNICATIONTYPE_HID,
                self.resources.reschkUseCommunicationType_value,
                97, 57, 0, 2, 7, 100), self)
        self.lstCommunicationType = self.insertComboBox(
            "lstCommunicationType",
            FaxWizardDialogConst.LSTCOMMUNICATIONTYPE_ACTION_PERFORMED,
            FaxWizardDialogConst.LSTCOMMUNICATIONTYPE_ITEM_CHANGED,
            FaxWizardDialogConst.LSTCOMMUNICATIONTYPE_TEXT_CHANGED,
            ("Dropdown", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
                (True, 12, FaxWizardDialogConst.LSTCOMMUNICATIONTYPE_HID,
                    105, 68, 2, 8, 174), self)
        self.chkUseSubject = self.insertCheckBox("chkUseSubject",
            FaxWizardDialogConst.CHKUSESUBJECT_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSESUBJECT_HID,
                self.resources.reschkUseSubject_value, 97, 87, 0, 2, 9, 212),
            self)
        self.chkUseSalutation = self.insertCheckBox("chkUseSalutation",
            FaxWizardDialogConst.CHKUSESALUTATION_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSESALUTATION_HID,
                self.resources.reschkUseSalutation_value,
                97, 102, 0, 2, 10, 100), self)
        self.lstSalutation = self.insertComboBox("lstSalutation",
            FaxWizardDialogConst.LSTSALUTATION_ACTION_PERFORMED,
            FaxWizardDialogConst.LSTSALUTATION_ITEM_CHANGED,
            FaxWizardDialogConst.LSTSALUTATION_TEXT_CHANGED,
            ("Dropdown", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (True, 12, FaxWizardDialogConst.LSTSALUTATION_HID,
                105, 113, 2, 11, 174), self)
        self.chkUseGreeting = self.insertCheckBox("chkUseGreeting",
            FaxWizardDialogConst.CHKUSEGREETING_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSEGREETING_HID,
                self.resources.reschkUseGreeting_value,
                97, 132, 0, 2, 12, 100), self)
        self.lstGreeting = self.insertComboBox("lstGreeting",
            FaxWizardDialogConst.LSTGREETING_ACTION_PERFORMED,
            FaxWizardDialogConst.LSTGREETING_ITEM_CHANGED,
            FaxWizardDialogConst.LSTGREETING_TEXT_CHANGED,
            ("Dropdown", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (True, 12, FaxWizardDialogConst.LSTGREETING_HID,
                105, 143, 2, 13, 174), self)
        self.chkUseFooter = self.insertCheckBox("chkUseFooter",
            FaxWizardDialogConst.CHKUSEFOOTER_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKUSEFOOTER_HID,
                self.resources.reschkUseFooter_value, 97, 163,
                0, 2, 14, 212), self)
        self.insertLabel("lblTitle3",
            ("FontDescriptor", PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor5, 16, self.resources.reslblTitle3_value,
                True, 91, 8, 2, 59, 212))

    def buildStep3(self):
        self.optSenderPlaceholder = self.insertRadioButton(
            "optSenderPlaceholder",
            FaxWizardDialogConst.OPTSENDERPLACEHOLDER_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTSENDERPLACEHOLDER_HID,
                self.resources.resoptSenderPlaceholder_value,
                104, 42, 3, 15, 149), self)
        self.optSenderDefine = self.insertRadioButton("optSenderDefine",
            FaxWizardDialogConst.OPTSENDERDEFINE_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTSENDERDEFINE_HID,
                self.resources.resoptSenderDefine_value,
                104, 54, 3, 16, 149), self)
        self.txtSenderName = self.insertTextField("txtSenderName",
            FaxWizardDialogConst.TXTSENDERNAME_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERNAME_HID,
                182, 67, 3, 17, 119), self)
        self.txtSenderStreet = self.insertTextField("txtSenderStreet",
            FaxWizardDialogConst.TXTSENDERSTREET_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERSTREET_HID,
                182, 81, 3, 18, 119), self)
        self.txtSenderPostCode = self.insertTextField("txtSenderPostCode",
            FaxWizardDialogConst.TXTSENDERPOSTCODE_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERPOSTCODE_HID,
                182, 95, 3, 19, 25), self)
        self.txtSenderState = self.insertTextField("txtSenderState",
            FaxWizardDialogConst.TXTSENDERSTATE_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERSTATE_HID,
                211, 95, 3, 20, 21), self)
        self.txtSenderCity = self.insertTextField("txtSenderCity",
            FaxWizardDialogConst.TXTSENDERCITY_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERCITY_HID,
                236, 95, 3, 21, 65), self)
        self.txtSenderFax = self.insertTextField("txtSenderFax",
            FaxWizardDialogConst.TXTSENDERFAX_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTSENDERFAX_HID,
                182, 109, 3, 22, 119), self)
        self.optReceiverPlaceholder = self.insertRadioButton(
            "optReceiverPlaceholder",
            FaxWizardDialogConst.OPTRECEIVERPLACEHOLDER_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTRECEIVERPLACEHOLDER_HID,
                self.resources.resoptReceiverPlaceholder_value,
                104, 148, 3, 23, 200), self)
        self.optReceiverDatabase = self.insertRadioButton(
            "optReceiverDatabase",
            FaxWizardDialogConst.OPTRECEIVERDATABASE_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTRECEIVERDATABASE_HID,
                self.resources.resoptReceiverDatabase_value,
                104, 160, 3, 24, 200), self)
        self.insertLabel("lblSenderAddress",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblSenderAddress_value,
                97, 28, 3, 46, 136))
        self.insertFixedLine("FixedLine2", (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (5, 90, 126, 3, 51, 212))
        self.insertLabel("lblSenderName",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblSenderName_value,
                113, 69, 3, 52, 68))
        self.insertLabel("lblSenderStreet",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblSenderStreet_value,
                113, 82, 3, 53, 68))
        self.insertLabel("lblPostCodeCity",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblPostCodeCity_value,
                113, 97, 3, 54, 68))
        self.insertLabel("lblTitle4",
            ("FontDescriptor",
                PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor5, 16, self.resources.reslblTitle4_value,
                 True, 91, 8, 3, 60, 212))
        self.insertLabel("lblSenderFax",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.resLabel1_value, 113, 111, 3, 68, 68))
        self.insertLabel("Label2",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.resLabel2_value, 97, 137, 3, 69, 136))

    def buildStep4(self):
        self.txtFooter = self.insertTextField("txtFooter",
            FaxWizardDialogConst.TXTFOOTER_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (47, FaxWizardDialogConst.TXTFOOTER_HID,
                True, 97, 40, 4, 25, 203), self)
        self.chkFooterNextPages = self.insertCheckBox("chkFooterNextPages",
            FaxWizardDialogConst.CHKFOOTERNEXTPAGES_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKFOOTERNEXTPAGES_HID,
                self.resources.reschkFooterNextPages_value,
                97, 92, 0, 4, 26, 202), self)
        self.chkFooterPageNumbers = self.insertCheckBox("chkFooterPageNumbers",
            FaxWizardDialogConst.CHKFOOTERPAGENUMBERS_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STATE,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.CHKFOOTERPAGENUMBERS_HID,
                self.resources.reschkFooterPageNumbers_value,
                97, 106, 0, 4, 27, 201), self)
        self.insertLabel("lblFooter",
            ("FontDescriptor",
                PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor4, 8, self.resources.reslblFooter_value,
                97, 28, 4, 33, 116))
        self.insertLabel("lblTitle5",
            ("FontDescriptor",
                PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor5, 16, self.resources.reslblTitle5_value,
                True, 91, 8, 4, 61, 212))

    def buildStep5(self):
        self.txtTemplateName = self.insertTextField("txtTemplateName",
            FaxWizardDialogConst.TXTTEMPLATENAME_TEXT_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                "Text",
                PropertyNames.PROPERTY_WIDTH),
            (12, FaxWizardDialogConst.TXTTEMPLATENAME_HID, 202, 56, 5, 28,
                self.resources.restxtTemplateName_value, 100), self)

        self.optCreateFax = self.insertRadioButton("optCreateFax",
            FaxWizardDialogConst.OPTCREATEFAX_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTCREATEFAX_HID,
                self.resources.resoptCreateFax_value,
                104, 111, 5, 30, 198), self)
        self.optMakeChanges = self.insertRadioButton("optMakeChanges",
            FaxWizardDialogConst.OPTMAKECHANGES_ITEM_CHANGED,
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_HELPURL,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, FaxWizardDialogConst.OPTMAKECHANGES_HID,
                self.resources.resoptMakeChanges_value,
                104, 123, 5, 31, 198), self)
        self.insertLabel("lblFinalExplanation1",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (28, self.resources.reslblFinalExplanation1_value,
                True, 97, 28, 5, 34, 205))
        self.insertLabel("lblProceed",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblProceed_value, 97, 100, 5,
                35, 204))
        self.insertLabel("lblFinalExplanation2",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (33, self.resources.reslblFinalExplanation2_value,
                True, 104, 145, 5, 36, 199))
        self.insertImage("ImageControl2",
            ("Border",
                PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_IMAGEURL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                "ScaleImage",
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (0, 10, UIConsts.INFOIMAGEURL, 92, 145,
                False, 5, 47, 10))
        self.insertLabel("lblTemplateName",
            (PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (8, self.resources.reslblTemplateName_value, 97, 58, 5,
                57, 101))

        self.insertLabel("lblTitle6",
            ("FontDescriptor",
                PropertyNames.PROPERTY_HEIGHT,
                PropertyNames.PROPERTY_LABEL,
                PropertyNames.PROPERTY_MULTILINE,
                PropertyNames.PROPERTY_POSITION_X,
                PropertyNames.PROPERTY_POSITION_Y,
                PropertyNames.PROPERTY_STEP,
                PropertyNames.PROPERTY_TABINDEX,
                PropertyNames.PROPERTY_WIDTH),
            (self.fontDescriptor5, 16, self.resources.reslblTitle6_value,
                True, 91, 8, 5, 62, 212))
