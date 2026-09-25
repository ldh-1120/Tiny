#pragma once

namespace tiny {
	enum class KeyCode {
        Unknown,

        Tab,
        Enter,
        Escape,
        Space,
        Backspace,
        DeleteKey,
        Insert,

        Home,
        End,
        PageUp,
        PageDown,

        Left,
        Right,
        Up,
        Down,

        Shift,
        Control,
        Alt,

        CapsLock,
        NumLock,
        ScrollLock,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Digit0,
        Digit1,
        Digit2,
        Digit3,
        Digit4,
        Digit5,
        Digit6,
        Digit7,
        Digit8,
        Digit9,

        Equal,
        Minus,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12
	};

    struct KeyboardModifiers {
        bool shift = false;
        bool control = false;
        bool alt = false;
    };

    struct KeyEvent {
        KeyCode key = KeyCode::Unknown;

        KeyboardModifiers modifiers;

        bool repeated = false;

        unsigned int repeatCount = 1;

        bool handled = false;
    };
}