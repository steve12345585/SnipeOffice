/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.libreoffice.kit;

import java.nio.ByteBuffer;

public class Office {
    private ByteBuffer handle;
    private MessageCallback messageCallback = null;

    public Office(ByteBuffer handle) {
        this.handle = handle;
        bindMessageCallback();
    }

    /**
     * Bind the signal callback in LOK.
     */
    private native void bindMessageCallback();

    public native String getError();

    private native ByteBuffer documentLoadNative(String url);

    public Document documentLoad(String url) {
        ByteBuffer documentHandle = documentLoadNative(url);
        Document document = null;
        if (documentHandle != null) {
            document = new Document(documentHandle);
        }
        return document;
    }

    public native void destroy();
    public native void destroyAndExit();
    public native void setDocumentPassword(String url, String pwd);
    public native void setOptionalFeatures(long options);

    public void setMessageCallback(MessageCallback messageCallback) {
        this.messageCallback = messageCallback;
    }

    /**
     * Callback triggered through JNI to indicate that a new signal
     * from LibreOfficeKit was retrieved.
     */
    private void messageRetrievedLOKit(int signalNumber, String payload) {
        if (messageCallback != null) {
            messageCallback.messageRetrieved(signalNumber, payload);
        }

    }

    /**
     * Callback to retrieve messages from LOK
     */
    public interface MessageCallback {
        /**
         * Invoked when a message is retrieved from LOK
         * @param signalNumber - signal type / number
         * @param payload - retrieved for the signal
         */
        void messageRetrieved(int signalNumber, String payload);
    }
}
