/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

using System;

namespace com.sun.star.uno
{
    public class UnoException : Exception
    {
        public IQueryInterface Context { get; set; }

        public UnoException() { }
        public UnoException(string Message, IQueryInterface Context)
            : base(Message) => this.Context = Context;
    }
}
