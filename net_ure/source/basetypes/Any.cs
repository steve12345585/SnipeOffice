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
    public class Any
    {
        public static readonly Any VOID = new Any(typeof(void), null);

        public Type Type { get; private set; }
        public Object Value { get; private set; }

        public Any(object value) => setValue(value?.GetType() ?? typeof(void), value);
        public Any(Type type, object value) => setValue(type, value);
        public static Any with<T>(T value) => new Any(typeof(T), value);

        public bool contains<T>()
        {
            if (typeof(IQueryInterface).IsAssignableFrom(Type))
            {
                if (typeof(T).IsAssignableFrom(Type))
                    // Special case for already implemented interface
                    return true;
                else if (typeof(IQueryInterface).IsAssignableFrom(typeof(T)))
                    // Special case for contained IQueryInterface
                    return ((IQueryInterface)Value).queryInterface(typeof(T)).hasValue();
            }
            return typeof(T).IsAssignableFrom(Type);
        }

        public T cast<T>()
        {
            if (typeof(IQueryInterface).IsAssignableFrom(Type))
            {
                if (typeof(T).IsAssignableFrom(Type))
                    // Special case for already implemented interface
                    return (T)Value;
                else if (typeof(IQueryInterface).IsAssignableFrom(typeof(T)))
                    // Special case for contained IQueryInterface
                    return (T)((IQueryInterface)Value).queryInterface(typeof(T)).Value;
            }
            return (T)Value;
        }

        public T castOrDefault<T>(T fallback = default)
        {
            if (typeof(IQueryInterface).IsAssignableFrom(Type))
            {
                if (typeof(T).IsAssignableFrom(Type))
                    // Special case for already implemented interface
                    return (T)Value;
                else if (typeof(IQueryInterface).IsAssignableFrom(typeof(T)))
                    // Special case for contained IQueryInterface
                    return (T)((IQueryInterface)Value).queryInterface(typeof(T)).Value;
            }
            return typeof(T).IsAssignableFrom(Type) ? (T)Value : fallback;
        }

        public bool hasValue() => Type != typeof(void);

        public void setValue(Type type, object value)
        {
            if (type is null)
                throw new ArgumentNullException(nameof(type), "Type of Any cannot be null.");

            if (type == typeof(Any))
                throw new ArgumentException("Any object cannot be nested inside another Any.");

            if (value is null && type != typeof(void))
                throw new ArgumentException("Value of Any can only be null if Type is void." +
                    " Perhaps you want Any.VOID?");

            Type = type;
            Value = value;
        }
        public void setValue(object value) => setValue(value.GetType(), value);
        public void setValue<T>(object value) => setValue(typeof(T), value);

        public bool equals(Any other)
        {
            return other != null && Type.Equals(other.Type)
                && (Value == null ? other.Value == null : Value.Equals(other.Value));
        }

        public static bool operator ==(Any left, Any right) => left?.Equals(right) ?? false;
        public static bool operator !=(Any left, Any right) => !(left == right);

        public override bool Equals(object obj) => obj is Any other && equals(other);
        public override int GetHashCode() => (Type, Value).GetHashCode();

        public override string ToString() => $"com.sun.star.uno.Any {{ Type = {Type}, Value = {Value ?? "Null"} }}";
    }
}
