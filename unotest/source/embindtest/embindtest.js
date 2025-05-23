/* -*- Mode: JS; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

Module.uno_init.then(function() {
    console.log('Running embindtest');
    let css = Module.uno.com.sun.star;
    let test = Module.uno.org.libreoffice.embindtest.Test.create(Module.getUnoComponentContext());
    console.assert(typeof test === 'object');
    {
        let v = test.getBoolean();
        console.log(v);
        console.assert(v === 1); //TODO: true
        console.assert(test.isBoolean(v));
    }
    {
        let v = test.getByte();
        console.log(v);
        console.assert(v === -12);
        console.assert(test.isByte(v));
    }
    {
        let v = test.getShort();
        console.log(v);
        console.assert(v === -1234);
        console.assert(test.isShort(v));
    }
    {
        let v = test.getUnsignedShort();
        console.log(v);
        console.assert(v === 54321);
        console.assert(test.isUnsignedShort(v));
    }
    {
        let v = test.getLong();
        console.log(v);
        console.assert(v === -123456);
        console.assert(test.isLong(v));
    }
    {
        let v = test.getUnsignedLong();
        console.log(v);
        console.assert(v === 3456789012);
        console.assert(test.isUnsignedLong(v));
    }
    {
        let v = test.getHyper();
        console.log(v);
        console.assert(v === -123456789n);
        console.assert(test.isHyper(v));
    }
    {
        let v = test.getUnsignedHyper();
        console.log(v);
        console.assert(v === 9876543210n);
        console.assert(test.isUnsignedHyper(v));
    }
    {
        let v = test.getFloat();
        console.log(v);
        console.assert(v === -10.25);
        console.assert(test.isFloat(v));
    }
    {
        let v = test.getDouble();
        console.log(v);
        console.assert(v === 100.5);
        console.assert(test.isDouble(v));
    }
    {
        let v = test.getChar();
        console.log(v);
        console.assert(v === 'Ö');
        console.assert(test.isChar(v));
    }
    {
        let v = test.getString();
        console.log(v);
        console.assert(v === 'hä');
        console.assert(test.isString(v));
    }
    {
        let v = test.getType();
        console.log(v);
        console.assert(v.toString() === 'long');
        console.assert(test.isType(v));
        console.assert(test.isType(Module.uno_Type.Long()));
    }
    {
        let v = test.getEnum();
        console.log(v);
        console.assert(v === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(test.isEnum(v));
    }
    {
        let v = test.getStruct();
        console.log(v);
        console.assert(v.m1 === 1); //TODO: true
        console.assert(v.m2 === -12);
        console.assert(v.m3 === -1234);
        console.assert(v.m4 === 54321);
        console.assert(v.m5 === -123456);
        console.assert(v.m6 === 3456789012);
        console.assert(v.m7 === -123456789n);
        console.assert(v.m8 === 9876543210n);
        console.assert(v.m9 === -10.25);
        console.assert(v.m10 === 100.5);
        console.assert(v.m11 === 'Ö');
        console.assert(v.m12 === 'hä');
        console.assert(v.m13.toString() === 'long');
        console.assert(v.m14.get() === -123456);
        console.assert(v.m15.size() === 3);
        console.assert(v.m15.get(0) === 'foo');
        console.assert(v.m15.get(1) === 'barr');
        console.assert(v.m15.get(2) === 'bazzz');
        console.assert(v.m16 === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v.m17.m === -123456);
        console.assert(v.m18.m1.m === 'foo');
        console.assert(v.m18.m2 === -123456);
        console.assert(v.m18.m3.get() === -123456);
        console.assert(v.m18.m4.m === 'barr');
        console.assert(Module.sameUnoObject(v.m19, test));
        console.assert(test.isStruct(v));
        v.m14.delete();
        v.m15.delete();
        v.m18.m3.delete();
    }
    {
        let v = test.getTemplate();
        console.log(v);
        console.assert(v.m1.m === 'foo');
        console.assert(v.m2 === -123456);
        console.assert(v.m3.get() === -123456);
        console.assert(v.m4.m === 'barr');
        console.assert(test.isTemplate(v));
        v.m3.delete();
    }
    {
        let v = test.getAnyVoid();
        console.log(v);
        console.assert(v.get() === undefined);
        console.assert(test.isAnyVoid(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Void(), undefined);
        console.assert(test.isAnyVoid(a));
        a.delete();
    }
    {
        let v = test.getAnyBoolean();
        console.log(v);
        console.assert(v.get() === true);
        console.assert(test.isAnyBoolean(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Boolean(), true);
        console.assert(test.isAnyBoolean(a));
        a.delete();
    }
    {
        let v = test.getAnyByte();
        console.log(v);
        console.assert(v.get() === -12);
        console.assert(test.isAnyByte(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Byte(), -12);
        console.assert(test.isAnyByte(a));
        a.delete();
    }
    {
        let v = test.getAnyShort();
        console.log(v);
        console.assert(v.get() === -1234);
        console.assert(test.isAnyShort(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Short(), -1234);
        console.assert(test.isAnyShort(a));
        a.delete();
    }
    {
        let v = test.getAnyUnsignedShort();
        console.log(v);
        console.assert(v.get() === 54321);
        console.assert(test.isAnyUnsignedShort(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.UnsignedShort(), 54321);
        console.assert(test.isAnyUnsignedShort(a));
        a.delete();
    }
    {
        let v = test.getAnyLong();
        console.log(v);
        console.assert(v.get() === -123456);
        console.assert(test.isAnyLong(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Long(), -123456);
        console.assert(test.isAnyLong(a));
        a.delete();
    }
    {
        let v = test.getAnyUnsignedLong();
        console.log(v);
        console.assert(v.get() === 3456789012);
        console.assert(test.isAnyUnsignedLong(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.UnsignedLong(), 3456789012);
        console.assert(test.isAnyUnsignedLong(a));
        a.delete();
    }
    {
        let v = test.getAnyHyper();
        console.log(v);
        console.assert(v.get() === -123456789n);
        console.assert(test.isAnyHyper(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Hyper(), -123456789n);
        console.assert(test.isAnyHyper(a));
        a.delete();
    }
    {
        let v = test.getAnyUnsignedHyper();
        console.log(v);
        console.assert(v.get() === 9876543210n);
        console.assert(test.isAnyUnsignedHyper(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.UnsignedHyper(), 9876543210n);
        console.assert(test.isAnyUnsignedHyper(a));
        a.delete();
    }
    {
        let v = test.getAnyFloat();
        console.log(v);
        console.assert(v.get() === -10.25);
        console.assert(test.isAnyFloat(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Float(), -10.25);
        console.assert(test.isAnyFloat(a));
        a.delete();
    }
    {
        let v = test.getAnyDouble();
        console.log(v);
        console.assert(v.get() === 100.5);
        console.assert(test.isAnyDouble(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Double(), 100.5);
        console.assert(test.isAnyDouble(a));
        a.delete();
    }
    {
        let v = test.getAnyChar();
        console.log(v);
        console.assert(v.get() === 'Ö');
        console.assert(test.isAnyChar(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Char(), 'Ö');
        console.assert(test.isAnyChar(a));
        a.delete();
    }
    {
        let v = test.getAnyString();
        console.log(v);
        console.assert(v.get() === 'hä');
        console.assert(test.isAnyString(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.String(), 'hä');
        console.assert(test.isAnyString(a));
        a.delete();
    }
    {
        let v = test.getAnyType();
        console.log(v);
        console.assert(v.get().toString() === 'long');
        console.assert(test.isAnyType(v));
        v.delete();
        let a = new Module.uno_Any(Module.uno_Type.Type(), Module.uno_Type.Long());
        console.assert(test.isAnyType(a));
        a.delete();
    }
    {
        let v = test.getAnySequence();
        console.log(v);
        let x = v.get();
        console.assert(x.size() === 3);
        console.assert(x.get(0) === 'foo');
        console.assert(x.get(1) === 'barr');
        console.assert(x.get(2) === 'bazzz');
        x.delete();
        console.assert(test.isAnySequence(v));
        v.delete();
        let s = new Module.uno_Sequence_string(["foo", "barr", "bazzz"]);
        let a = new Module.uno_Any(Module.uno_Type.Sequence(Module.uno_Type.String()), s);
        console.assert(test.isAnySequence(a));
        a.delete();
        s.delete();
    }
    {
        let v = test.getAnyEnum();
        console.log(v);
        console.assert(v.get() === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(test.isAnyEnum(v));
        v.delete();
        let a = new Module.uno_Any(
            Module.uno_Type.Enum('org.libreoffice.embindtest.Enum'),
            Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(test.isAnyEnum(a));
        a.delete();
    }
    {
        let v = test.getAnyStruct();
        console.log(v);
        console.assert(v.get().m1 === 1); //TODO: true
        console.assert(v.get().m2 === -12);
        console.assert(v.get().m3 === -1234);
        console.assert(v.get().m4 === 54321);
        console.assert(v.get().m5 === -123456);
        console.assert(v.get().m6 === 3456789012);
        console.assert(v.get().m7 === -123456789n);
        console.assert(v.get().m8 === 9876543210n);
        console.assert(v.get().m9 === -10.25);
        console.assert(v.get().m10 === 100.5);
        console.assert(v.get().m11 === 'Ö');
        console.assert(v.get().m12 === 'hä');
        console.assert(v.get().m13.toString() === 'long');
        console.assert(v.get().m14.get() === -123456);
        console.assert(v.get().m15.size() === 3);
        console.assert(v.get().m15.get(0) === 'foo');
        console.assert(v.get().m15.get(1) === 'barr');
        console.assert(v.get().m15.get(2) === 'bazzz');
        console.assert(v.get().m16 === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v.get().m17.m === -123456);
        console.assert(v.get().m18.m1.m === 'foo');
        console.assert(v.get().m18.m2 === -123456);
        console.assert(v.get().m18.m3.get() === -123456);
        console.assert(v.get().m18.m4.m === 'barr');
        console.assert(Module.sameUnoObject(v.get().m19, test));
        console.assert(test.isAnyStruct(v));
        v.get().m14.delete();
        v.get().m15.delete();
        v.get().m18.m3.delete();
        v.delete();
        let m14 = new Module.uno_Any(Module.uno_Type.Long(), -123456);
        let m15 = new Module.uno_Sequence_string(["foo", "barr", "bazzz"]);
        let m18m3 = new Module.uno_Any(Module.uno_Type.Long(), -123456);
        let a = new Module.uno_Any(
            Module.uno_Type.Struct('org.libreoffice.embindtest.Struct'),
            {m1: true, m2: -12, m3: -1234, m4: 54321, m5: -123456, m6: 3456789012, m7: -123456789n,
             m8: 9876543210n, m9: -10.25, m10: 100.5, m11: 'Ö', m12: 'hä',
             m13: Module.uno_Type.Long(), m14, m15,
             m16: Module.uno.org.libreoffice.embindtest.Enum.E_2, m17: {m: -123456},
             m18: {m1: {m: 'foo'}, m2: -123456, m3: m18m3, m4: {m: 'barr'}}, m19: test});
        console.assert(test.isAnyStruct(a));
        m14.delete();
        m15.delete();
        m18m3.delete();
        a.delete();
    }
    {
        let v = test.getAnyException();
        console.log(v);
        console.assert(v.get().Message.startsWith('error'));
        console.assert(v.get().Context === null);
        console.assert(v.get().m1 === -123456);
        console.assert(v.get().m2 === 100.5);
        console.assert(v.get().m3 === 'hä');
        console.assert(test.isAnyException(v));
        v.delete();
        let a = new Module.uno_Any(
            Module.uno_Type.Exception('org.libreoffice.embindtest.Exception'),
            {Message: 'error', Context: null, m1: -123456, m2: 100.5, m3: 'hä'});
        console.assert(test.isAnyException(a));
        a.delete();
    }
    {
        let v = test.getAnyInterface();
        console.log(v);
        console.assert(Module.sameUnoObject(v.get(), test));
        console.assert(test.isAnyInterface(v));
        v.delete();
        let a = new Module.uno_Any(
            Module.uno_Type.Interface('org.libreoffice.embindtest.XTest'), test);
        console.assert(test.isAnyInterface(a));
        a.delete();
    }
    {
        let v = test.getSequenceBoolean();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 1); //TODO: true
        console.assert(v.get(1) === 1); //TODO: true
        console.assert(v.get(2) === 0); //TODO: false
        console.assert(test.isSequenceBoolean(v));
        v.delete();
    }
    {
        let v = test.getSequenceByte();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -12);
        console.assert(v.get(1) === 1);
        console.assert(v.get(2) === 12);
        console.assert(test.isSequenceByte(v));
        v.delete();
    }
    {
        let v = test.getSequenceShort();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -1234);
        console.assert(v.get(1) === 1);
        console.assert(v.get(2) === 1234);
        console.assert(test.isSequenceShort(v));
        v.delete();
    }
    {
        let v = test.getSequenceUnsignedShort();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 1);
        console.assert(v.get(1) === 10);
        console.assert(v.get(2) === 54321);
        console.assert(test.isSequenceUnsignedShort(v));
        v.delete();
    }
    {
        let v = test.getSequenceLong();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -123456);
        console.assert(v.get(1) === 1);
        console.assert(v.get(2) === 123456);
        console.assert(test.isSequenceLong(v));
        v.delete();
    }
    {
        let v = test.getSequenceUnsignedLong();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 1);
        console.assert(v.get(1) === 10);
        console.assert(v.get(2) === 3456789012);
        console.assert(test.isSequenceUnsignedLong(v));
        v.delete();
    }
    {
        let v = test.getSequenceHyper();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -123456789n);
        console.assert(v.get(1) === 1n);
        console.assert(v.get(2) === 123456789n);
        console.assert(test.isSequenceHyper(v));
        v.delete();
    }
    {
        let v = test.getSequenceUnsignedHyper();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 1n);
        console.assert(v.get(1) === 10n);
        console.assert(v.get(2) === 9876543210n);
        console.assert(test.isSequenceUnsignedHyper(v));
        v.delete();
    }
    {
        let v = test.getSequenceFloat();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -10.25);
        console.assert(v.get(1) === 1.5);
        console.assert(v.get(2) === 10.75);
        console.assert(test.isSequenceFloat(v));
        v.delete();
    }
    {
        let v = test.getSequenceDouble();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === -100.5);
        console.assert(v.get(1) === 1.25);
        console.assert(v.get(2) === 100.75);
        console.assert(test.isSequenceDouble(v));
        v.delete();
    }
    {
        let v = test.getSequenceChar();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 'a');
        console.assert(v.get(1) === 'B');
        console.assert(v.get(2) === 'Ö');
        console.assert(test.isSequenceChar(v));
        v.delete();
    }
    {
        let v = test.getSequenceString();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === 'foo');
        console.assert(v.get(1) === 'barr');
        console.assert(v.get(2) === 'bazzz');
        console.assert(test.isSequenceString(v));
        v.delete();
    }
    {
        let v = test.getSequenceType();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0).toString() === 'long');
        console.assert(v.get(1).toString() === 'void');
        console.assert(v.get(2).toString() === '[]org.libreoffice.embindtest.Enum');
        console.assert(test.isSequenceType(v));
        v.delete();
        let s = new Module.uno_Sequence_type([
            Module.uno_Type.Long(), Module.uno_Type.Void(),
            Module.uno_Type.Sequence(Module.uno_Type.Enum('org.libreoffice.embindtest.Enum'))]);
        console.assert(test.isSequenceType(s));
        s.delete();
    }
    {
        let v = test.getSequenceAny();
        console.log(v);
        console.assert(v.size() === 3);
        let e0 = v.get(0);
        console.assert(e0.get() === -123456);
        e0.delete();
        let e1 = v.get(1);
        console.assert(e1.get() === undefined);
        e1.delete();
        let e2 = v.get(2);
        let s = e2.get();
        console.assert(s.size() === 3);
        console.assert(s.get(0) === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(s.get(1) === Module.uno.org.libreoffice.embindtest.Enum.E3);
        console.assert(s.get(2) === Module.uno.org.libreoffice.embindtest.Enum.E_10);
        s.delete();
        e2.delete();
        console.assert(test.isSequenceAny(v));
        v.delete();
    }
    {
        let v = test.getSequenceSequenceString();
        console.log(v);
        console.assert(v.size() === 3);
        let e0 = v.get(0);
        console.assert(e0.size() === 0);
        e0.delete();
        let e1 = v.get(1);
        console.assert(e1.size() === 2);
        console.assert(e1.get(0) === 'foo');
        console.assert(e1.get(1) === 'barr');
        e1.delete();
        let e2 = v.get(2);
        console.assert(e2.size() === 1);
        console.assert(e2.get(0) === 'baz');
        e2.delete();
        console.assert(test.isSequenceSequenceString(v));
        v.delete();
    }
    {
        let v = test.getSequenceEnum();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0) === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v.get(1) === Module.uno.org.libreoffice.embindtest.Enum.E3);
        console.assert(v.get(2) === Module.uno.org.libreoffice.embindtest.Enum.E_10);
        console.assert(test.isSequenceEnum(v));
        v.delete();
    }
    {
        let v = test.getSequenceStruct();
        console.log(v);
        console.assert(v.size() === 3);
        console.assert(v.get(0).m1 === 1); //TODO: true
        console.assert(v.get(0).m2 === -12);
        console.assert(v.get(0).m3 === -1234);
        console.assert(v.get(0).m4 === 1);
        console.assert(v.get(0).m5 === -123456);
        console.assert(v.get(0).m6 === 1);
        console.assert(v.get(0).m7 === -123456789n);
        console.assert(v.get(0).m8 === 1n);
        console.assert(v.get(0).m9 === -10.25);
        console.assert(v.get(0).m10 === -100.5);
        console.assert(v.get(0).m11 === 'a');
        console.assert(v.get(0).m12 === 'hä');
        console.assert(v.get(0).m13.toString() === 'long');
        console.assert(v.get(0).m14.get() === -123456);
        console.assert(v.get(0).m15.size() === 0);
        console.assert(v.get(0).m16 === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v.get(0).m17.m === -123456);
        console.assert(v.get(0).m18.m1.m === 'foo');
        console.assert(v.get(0).m18.m2 === -123456);
        console.assert(v.get(0).m18.m3.get() === -123456);
        console.assert(v.get(0).m18.m4.m === 'barr');
        console.assert(Module.sameUnoObject(v.get(0).m19, test));
        console.assert(v.get(1).m1 === 1); //TODO: true
        console.assert(v.get(1).m2 === 1);
        console.assert(v.get(1).m3 === 1);
        console.assert(v.get(1).m4 === 10);
        console.assert(v.get(1).m5 === 1);
        console.assert(v.get(1).m6 === 10);
        console.assert(v.get(1).m7 === 1n);
        console.assert(v.get(1).m8 === 10n);
        console.assert(v.get(1).m9 === 1.5);
        console.assert(v.get(1).m10 === 1.25);
        console.assert(v.get(1).m11 === 'B');
        console.assert(v.get(1).m12 === 'barr');
        console.assert(v.get(1).m13.toString() === 'void');
        console.assert(v.get(1).m14.get() === undefined);
        console.assert(v.get(1).m15.size() === 2);
        console.assert(v.get(1).m15.get(0) === 'foo');
        console.assert(v.get(1).m15.get(1) === 'barr');
        console.assert(v.get(1).m16 === Module.uno.org.libreoffice.embindtest.Enum.E3);
        console.assert(v.get(1).m17.m === 1);
        console.assert(v.get(1).m18.m1.m === 'baz');
        console.assert(v.get(1).m18.m2 === 1);
        console.assert(v.get(1).m18.m3.get() === undefined);
        console.assert(v.get(1).m18.m4.m === 'foo');
        console.assert(v.get(1).m19 === null);
        console.assert(v.get(2).m1 === 0); //TODO: false
        console.assert(v.get(2).m2 === 12);
        console.assert(v.get(2).m3 === 1234);
        console.assert(v.get(2).m4 === 54321);
        console.assert(v.get(2).m5 === 123456);
        console.assert(v.get(2).m6 === 3456789012);
        console.assert(v.get(2).m7 === 123456789n);
        console.assert(v.get(2).m8 === 9876543210n);
        console.assert(v.get(2).m9 === 10.75);
        console.assert(v.get(2).m10 === 100.75);
        console.assert(v.get(2).m11 === 'Ö');
        console.assert(v.get(2).m12 === 'bazzz');
        console.assert(v.get(2).m13.toString() === '[]org.libreoffice.embindtest.Enum');
        console.assert(v.get(2).m14.get().size() === 3);
        console.assert(
            v.get(2).m14.get().get(0) === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v.get(2).m14.get().get(1) === Module.uno.org.libreoffice.embindtest.Enum.E3);
        console.assert(
            v.get(2).m14.get().get(2) === Module.uno.org.libreoffice.embindtest.Enum.E_10);
        console.assert(v.get(2).m15.size() === 1);
        console.assert(v.get(2).m15.get(0) === 'baz');
        console.assert(v.get(2).m16 === Module.uno.org.libreoffice.embindtest.Enum.E_10);
        console.assert(v.get(2).m17.m === 123456);
        console.assert(v.get(2).m18.m1.m === 'barr');
        console.assert(v.get(2).m18.m2 === 123456);
        console.assert(v.get(2).m18.m3.get().size() === 3);
        console.assert(
            v.get(2).m18.m3.get().get(0) === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(
            v.get(2).m18.m3.get().get(1) === Module.uno.org.libreoffice.embindtest.Enum.E3);
        console.assert(
            v.get(2).m18.m3.get().get(2) === Module.uno.org.libreoffice.embindtest.Enum.E_10);
        console.assert(v.get(2).m18.m4.m === 'bazz');
        console.assert(Module.sameUnoObject(v.get(2).m19, test));
        console.assert(test.isSequenceStruct(v));
        v.get(0).m14.delete();
        v.get(0).m15.delete();
        v.get(0).m18.m3.delete();
        v.get(1).m14.delete();
        v.get(1).m15.delete();
        v.get(1).m18.m3.delete();
        v.get(2).m14.delete();
        v.get(2).m15.delete();
        v.get(2).m18.m3.get().delete();
        v.get(2).m18.m3.delete();
        v.delete();
    }
    {
        let v = test.getNull();
        console.log(v);
        console.assert(v === null);
        console.assert(test.isNull(v));
    }
    {
        let v = css.task.XJob.query(test);
        console.log(v);
        console.assert(v === null);
    }
    {
        const v1 = new Module.uno_InOutParam_boolean;
        const v2 = new Module.uno_InOutParam_byte;
        const v3 = new Module.uno_InOutParam_short;
        const v4 = new Module.uno_InOutParam_unsigned_short;
        const v5 = new Module.uno_InOutParam_long;
        const v6 = new Module.uno_InOutParam_unsigned_long;
        const v7 = new Module.uno_InOutParam_hyper;
        const v8 = new Module.uno_InOutParam_unsigned_hyper;
        const v9 = new Module.uno_InOutParam_float;
        const v10 = new Module.uno_InOutParam_double;
        const v11 = new Module.uno_InOutParam_char;
        const v12 = new Module.uno_InOutParam_string;
        const v13 = new Module.uno_InOutParam_type;
        const v14 = new Module.uno_InOutParam_any;
        const v15 = new Module.uno_InOutParam_sequence_string;
        const v16 = new Module.uno_InOutParam_org$libreoffice$embindtest$Enum;
        const v17 = new Module.uno_InOutParam_org$libreoffice$embindtest$Struct;
        const v18 = new Module.uno_InOutParam_org$libreoffice$embindtest$XTest;
        test.getOut(
            v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18);
        console.log(v1.val);
        console.log(v2.val);
        console.log(v3.val);
        console.log(v4.val);
        console.log(v5.val);
        console.log(v6.val);
        console.log(v7.val);
        console.log(v8.val);
        console.log(v9.val);
        console.log(v10.val);
        console.log(v11.val);
        console.log(v12.val);
        console.log(v13.val);
        console.log(v14.val);
        console.log(v15.val);
        console.log(v16.val);
        console.log(v17.val);
        console.log(v18.val);
        console.assert(v1.val === 1); //TODO: true
        console.assert(v2.val === -12);
        console.assert(v3.val === -1234);
        console.assert(v4.val === 54321);
        console.assert(v5.val === -123456);
        console.assert(v6.val === 3456789012);
        console.assert(v7.val === -123456789n);
        console.assert(v8.val === 9876543210n);
        console.assert(v9.val === -10.25);
        console.assert(v10.val === 100.5);
        console.assert(v11.val === 'Ö');
        console.assert(v12.val === 'hä');
        console.assert(v13.val.toString() === 'long');
        console.assert(v14.val.get() === -123456)
        console.assert(v15.val.size() === 3);
        console.assert(v15.val.get(0) === 'foo');
        console.assert(v15.val.get(1) === 'barr');
        console.assert(v15.val.get(2) === 'bazzz');
        console.assert(v16.val === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v17.val.m1 === 1); //TODO: true
        console.assert(v17.val.m2 === -12);
        console.assert(v17.val.m3 === -1234);
        console.assert(v17.val.m4 === 54321);
        console.assert(v17.val.m5 === -123456);
        console.assert(v17.val.m6 === 3456789012);
        console.assert(v17.val.m7 === -123456789n);
        console.assert(v17.val.m8 === 9876543210n);
        console.assert(v17.val.m9 === -10.25);
        console.assert(v17.val.m10 === 100.5);
        console.assert(v17.val.m11 === 'Ö');
        console.assert(v17.val.m12 === 'hä');
        console.assert(v17.val.m13.toString() === 'long');
        console.assert(v17.val.m14.get() === -123456);
        console.assert(v17.val.m15.size() === 3);
        console.assert(v17.val.m15.get(0) === 'foo');
        console.assert(v17.val.m15.get(1) === 'barr');
        console.assert(v17.val.m15.get(2) === 'bazzz');
        console.assert(v17.val.m16 === Module.uno.org.libreoffice.embindtest.Enum.E_2);
        console.assert(v17.val.m17.m === -123456);
        console.assert(v17.val.m18.m1.m === 'foo');
        console.assert(v17.val.m18.m2 === -123456);
        console.assert(v17.val.m18.m3.get() === -123456);
        console.assert(v17.val.m18.m4.m === 'barr');
        console.assert(Module.sameUnoObject(v17.val.m19, test));
        console.assert(Module.sameUnoObject(v18.val, test));
        v1.delete();
        v2.delete();
        v3.delete();
        v4.delete();
        v5.delete();
        v6.delete();
        v7.delete();
        v8.delete();
        v9.delete();
        v10.delete();
        v11.delete();
        v12.delete();
        v13.delete();
        v14.val.delete();
        v14.delete();
        v15.val.delete();
        v15.delete();
        v16.delete();
        v17.val.m14.delete();
        v17.val.m15.delete();
        v17.val.m18.m3.delete();
        v17.delete();
        v18.delete();
    }
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Boolean === true);
    console.assert(test.isBoolean(Module.uno.org.libreoffice.embindtest.Constants.Boolean));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Byte === -12);
    console.assert(test.isByte(Module.uno.org.libreoffice.embindtest.Constants.Byte));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Short === -1234);
    console.assert(test.isShort(Module.uno.org.libreoffice.embindtest.Constants.Short));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.UnsignedShort === 54321);
    console.assert(
        test.isUnsignedShort(Module.uno.org.libreoffice.embindtest.Constants.UnsignedShort));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Long === -123456);
    console.assert(test.isLong(Module.uno.org.libreoffice.embindtest.Constants.Long));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.UnsignedLong === 3456789012);
    console.assert(
        test.isUnsignedLong(Module.uno.org.libreoffice.embindtest.Constants.UnsignedLong));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Hyper === -123456789n);
    console.assert(test.isHyper(Module.uno.org.libreoffice.embindtest.Constants.Hyper));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.UnsignedHyper === 9876543210n);
    console.assert(
        test.isUnsignedHyper(Module.uno.org.libreoffice.embindtest.Constants.UnsignedHyper));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Float === -10.25);
    console.assert(test.isFloat(Module.uno.org.libreoffice.embindtest.Constants.Float));
    console.assert(Module.uno.org.libreoffice.embindtest.Constants.Double === 100.5);
    console.assert(test.isDouble(Module.uno.org.libreoffice.embindtest.Constants.Double));
    try {
        test.throwRuntimeException();
        console.assert(false);
    } catch (e) {
        const any = Module.catchUnoException(e);
        console.assert(any.getType() == 'com.sun.star.uno.RuntimeException');
        const exc = any.get();
        console.assert(exc.Message.startsWith('test'));
        any.delete();
    }
    try {
        const wrapped = new Module.uno_Any(
            Module.uno_Type.Exception('com.sun.star.uno.RuntimeException'),
            {Message: 'test', Context: test});
        Module.throwUnoException(
            Module.uno_Type.Exception('com.sun.star.lang.WrappedTargetException'),
            {Message: 'wrapped', Context: test, TargetException: wrapped}, [wrapped]);
        console.assert(false);
    } catch (e) {
        const any = Module.catchUnoException(e);
        console.assert(any.getType() == 'com.sun.star.lang.WrappedTargetException');
        const exc = any.get();
        console.assert(exc.Message.startsWith('wrapped'));
        console.assert(Module.sameUnoObject(exc.Context, test));
        const wrappedAny = exc.TargetException;
        console.assert(wrappedAny.getType() == 'com.sun.star.uno.RuntimeException');
        const wrappedExc = wrappedAny.get();
        console.assert(wrappedExc.Message.startsWith('test'));
        console.assert(Module.sameUnoObject(wrappedExc.Context, test));
        any.delete();
        wrappedAny.delete();
    }
    const obj = Module.unoObject(
        ['com.sun.star.task.XJob', 'com.sun.star.task.XJobExecutor',
         'org.libreoffice.embindtest.XAttributes'],
        {
            execute(args) {
                if (args.size() !== 1 || args.get(0).Name !== 'name') {
                    Module.throwUnoException(
                        Module.uno_Type.Exception('com.sun.star.lang.IllegalArgumentException'),
                        {Message: 'bad args', Context: null, ArgumentPosition: 0}, []);
                }
                console.log('Hello ' + args.get(0).Value.get());
                return new Module.uno_Any(Module.uno_Type.Void(), undefined);
            },
            trigger(event) { console.log('Ola ' + event); },
            the_LongAttribute: -123456,
            getLongAttribute() { return this.the_LongAttribute; },
            setLongAttribute(value) { this.the_LongAttribute = value; },
            the_StringAttribute: 'hä',
            getStringAttribute() { return this.the_StringAttribute; },
            setStringAttribute(value) { this.the_StringAttribute = value; },
            getReadOnlyAttribute() { return true; }
        });
    {
        const s = css.lang.XTypeProvider.query(obj).getTypes();
        console.assert(s.size() === 4);
        console.assert(s.get(0).toString() === 'com.sun.star.lang.XTypeProvider');
        console.assert(s.get(1).toString() === 'com.sun.star.task.XJob');
        console.assert(s.get(2).toString() === 'com.sun.star.task.XJobExecutor');
        console.assert(s.get(3).toString() === 'org.libreoffice.embindtest.XAttributes');
        s.delete();
    }
    {
        const s = css.lang.XTypeProvider.query(obj).getImplementationId();
        console.assert(s.size() === 0);
        s.delete();
    }
    test.passJob(css.task.XJob.query(obj));
    test.passJobExecutor(css.task.XJobExecutor.query(obj), false);
    //TODO: test.passJobExecutor(css.task.XJobExecutor.query(obj), true);
    test.passInterface(obj);
    css.task.XJobExecutor.query(obj).trigger('from JS');
    {
        const attrs = Module.uno.org.libreoffice.embindtest.XAttributes.query(obj);
        console.assert(attrs.LongAttribute === -123456);
        attrs.LongAttribute = 789;
        console.assert(attrs.LongAttribute === 789);
        console.assert(attrs.StringAttribute === 'hä');
        attrs.StringAttribute = 'foo';
        console.assert(attrs.StringAttribute === 'foo');
        console.assert(attrs.ReadOnlyAttribute === 1); //TODO: true
        try {
            attrs.ReadOnlyAttribute = false;
            console.assert(false);
        } catch (e) {}
        console.assert(test.checkAttributes(attrs));
    }
    console.assert(test.StringAttribute === 'hä');
    test.StringAttribute = 'foo';
    console.assert(test.StringAttribute === 'foo');
    console.assert(test.testSolarMutex());

    const args = new Module.uno_Sequence_any(
        [new Module.uno_Any(Module.uno_Type.Interface('com.sun.star.uno.XInterface'), test)]);
    const invoke = css.script.XInvocation2.query(css.script.Invocation.create(
        Module.getUnoComponentContext()).createInstanceWithArguments(args));
    args.get(0).delete();
    args.delete();
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getBoolean', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isBoolean', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getByte', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isByte', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getShort', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isShort', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getUnsignedShort', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isUnsignedShort', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getLong', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isLong', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getUnsignedLong', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isUnsignedLong', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getHyper', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isHyper', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getUnsignedHyper', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isUnsignedHyper', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getFloat', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isFloat', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getDouble', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isDouble', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getChar', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isChar', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getString', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isString', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getType', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isType', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getEnum', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isEnum', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getStruct', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isStruct', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getStructLong', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isStructLong', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getStructString', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isStructString', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getAnyLong', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isAnyLong', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getSequenceLong', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isSequenceLong', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params1 = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret1 = invoke.invoke('getNull', params1, outparamindex, outparam);
        console.log(ret1.get());
        const params2 = new Module.uno_Sequence_any([ret1]);
        const ret2 = invoke.invoke('isNull', params2, outparamindex, outparam);
        console.log(ret2.get());
        console.assert(ret2.get());
        ret1.delete();
        params1.delete();
        ret2.delete();
        params2.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const params = new Module.uno_Sequence_any(18, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        const ret = invoke.invoke('getOut', params, outparamindex, outparam);
        console.assert(ret.get() === undefined);
        ret.delete();
        params.delete();
        console.assert(outparamindex.val.size() == 18);
        outparamindex.val.delete();
        outparamindex.delete();
        console.assert(outparam.val.size() == 18);
        console.assert(test.isBoolean(outparam.val.get(0).get()));
        outparam.val.get(0).delete();
        console.assert(test.isByte(outparam.val.get(1).get()));
        outparam.val.get(1).delete();
        console.assert(test.isShort(outparam.val.get(2).get()));
        outparam.val.get(2).delete();
        console.assert(test.isUnsignedShort(outparam.val.get(3).get()));
        outparam.val.get(3).delete();
        console.assert(test.isLong(outparam.val.get(4).get()));
        outparam.val.get(4).delete();
        console.assert(test.isUnsignedLong(outparam.val.get(5).get()));
        outparam.val.get(5).delete();
        console.assert(test.isHyper(outparam.val.get(6).get()));
        outparam.val.get(6).delete();
        console.assert(test.isUnsignedHyper(outparam.val.get(7).get()));
        outparam.val.get(7).delete();
        console.assert(test.isFloat(outparam.val.get(8).get()));
        outparam.val.get(8).delete();
        console.assert(test.isDouble(outparam.val.get(9).get()));
        outparam.val.get(9).delete();
        console.assert(test.isChar(outparam.val.get(10).get()));
        outparam.val.get(10).delete();
        console.assert(test.isString(outparam.val.get(11).get()));
        outparam.val.get(11).delete();
        console.assert(test.isType(outparam.val.get(12).get()));
        outparam.val.get(12).delete();
        console.assert(test.isAnyLong(outparam.val.get(13)));
        outparam.val.get(13).delete();
        console.assert(test.isSequenceString(outparam.val.get(14).get()));
        outparam.val.get(14).get().delete();
        outparam.val.get(14).delete();
        console.assert(test.isEnum(outparam.val.get(15).get()));
        outparam.val.get(15).delete();
        console.assert(test.isStruct(outparam.val.get(16).get()));
        outparam.val.get(16).delete();
        console.assert(Module.sameUnoObject(outparam.val.get(17).get(), test));
        outparam.val.get(17).delete();
        outparam.val.delete();
        outparam.delete();
    }
    {
        const params = new Module.uno_Sequence_any(0, Module.uno_Sequence.FromSize);
        const outparamindex = new Module.uno_InOutParam_sequence_short;
        const outparam = new Module.uno_InOutParam_sequence_any;
        try {
            const ret = invoke.invoke('throwRuntimeException', params, outparamindex, outparam);
            console.assert(false);
            ret.delete();
        } catch (e) {
            const any = Module.catchUnoException(e);
            console.assert(any.getType() == 'com.sun.star.reflection.InvocationTargetException');
            const target = any.get().TargetException;
            console.assert(target.getType() == 'com.sun.star.uno.RuntimeException');
            const exc = target.get();
            console.assert(exc.Message.startsWith('test'));
            any.delete();
            target.delete();
        }
        params.delete();
        outparamindex.delete();
        outparam.delete();
    }
    {
        const ret1 = invoke.getValue('StringAttribute');
        console.assert(ret1.get() === 'foo');
        ret1.delete();
        let a = new Module.uno_Any(Module.uno_Type.String(), 'bar');
        invoke.setValue('StringAttribute', a);
        a.delete();
        const ret2 = invoke.getValue('StringAttribute');
        console.assert(ret2.get() === 'bar');
        ret2.delete();
    }
    {
        const args = new Module.uno_Sequence_com$sun$star$beans$NamedValue(
            0, Module.uno_Sequence.FromSize);
        const ret =
              Module.uno.org.libreoffice.embindtest.BridgeTest(Module.getUnoComponentContext()).
              execute(args);
        args.delete();
        console.assert(ret.get() === true);
        ret.delete();
    }
});

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
