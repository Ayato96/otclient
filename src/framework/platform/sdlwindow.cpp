/*
 * Copyright (c) 2010-2013 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "sdlwindow.h"
#include <framework/graphics/image.h>
#include <framework/core/graphicalapplication.h>

SDLWindow::SDLWindow()
{
    m_window = NULL;
    m_renderer = NULL;
    m_minimumSize = Size(600,480);
    m_size = Size(600,480);
}

void SDLWindow::init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

#ifdef OPENGL_ES
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_ES);
#endif

#ifdef MOBILE
    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN;
#else
    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
#endif

    m_window = SDL_CreateWindow(g_app.getName().c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                m_size.width(), m_size.height(),
                                flags);

    if(!m_window)
        g_logger.fatal("Unable to create SDL window");

    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    m_size = Size(w,h);

    m_context = SDL_GL_CreateContext(m_window);
    if(!m_context)
        g_logger.fatal("Unable to create SDL GL context");
    SDL_GL_MakeCurrent(m_window, m_context);
}

void SDLWindow::terminate()
{
    SDL_GL_DeleteContext(m_context);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void SDLWindow::move(const Point& pos)
{
    if(pos.x < 0 || pos.y < 0)
        return;

    SDL_SetWindowPosition(m_window, pos.x, pos.y);
}

void SDLWindow::resize(const Size& size)
{
    if(!size.isValid())
        return;
    m_size = size;
    SDL_SetWindowSize(m_window, m_size.width(), m_size.height());
    if(m_onResize)
        m_onResize(m_size);
}

void SDLWindow::show()
{
    SDL_ShowWindow(m_window);
}

void SDLWindow::hide()
{
    SDL_HideWindow(m_window);
}

void SDLWindow::maximize()
{
    SDL_MaximizeWindow(m_window);
}

Fw::MouseButton translateMouseButton(uint8 sdlButton)
{
    switch(sdlButton) {
        case SDL_BUTTON_LEFT:
            return Fw::MouseLeftButton;
            break;
        case SDL_BUTTON_MIDDLE:
            return Fw::MouseMidButton;
            break;
        case SDL_BUTTON_RIGHT:
            return Fw::MouseRightButton;
    }
    return Fw::MouseNoButton;
}

void SDLWindow::poll()
{
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_WINDOWEVENT: {
            switch(event.window.event) {
            case SDL_WINDOWEVENT_SHOWN:
                m_visible = true;
                break;
            case SDL_WINDOWEVENT_HIDDEN:
                m_visible = false;
                break;
            case SDL_WINDOWEVENT_MOVED:
                m_position = Point(event.window.data1, event.window.data2);
                break;
            case SDL_WINDOWEVENT_RESIZED:
                g_logger.info(stdext::format("resize %d %d", event.window.data1, event.window.data2));
                m_size = Size(event.window.data1, event.window.data2);
                if(m_onResize)
                    m_onResize(m_size);
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                m_maximized = false;
                m_visible = false;
                break;
            case SDL_WINDOWEVENT_MAXIMIZED:
                m_maximized = true;
                m_visible = true;
                break;
            case SDL_WINDOWEVENT_RESTORED:
                m_maximized = false;
                m_visible = true;
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                m_focused = true;
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                m_focused = false;
                break;
            case SDL_WINDOWEVENT_CLOSE:
                break;
            }
            break;
        }
        case SDL_KEYDOWN:
            break;
        case SDL_KEYUP:
            break;
        case SDL_TEXTINPUT:
            m_inputEvent.reset(Fw::KeyTextInputEvent);
            m_inputEvent.keyText = event.text.text;
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        case SDL_MOUSEMOTION:
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseMoveInputEvent;
            m_inputEvent.mouseMoved = Point(event.motion.xrel, event.motion.yrel);
            m_inputEvent.mousePos = Point(event.motion.x, event.motion.y);
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        case SDL_MOUSEBUTTONDOWN: {
            Fw::MouseButton button = translateMouseButton(event.button.button);
            if(button != Fw::MouseNoButton) {
                m_inputEvent.reset();
                m_inputEvent.type = Fw::MousePressInputEvent;
                m_inputEvent.mouseButton = button;
                m_mouseButtonStates[button] = true;
                if(m_onInputEvent)
                    m_onInputEvent(m_inputEvent);
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            Fw::MouseButton button = translateMouseButton(event.button.button);
            if(button == Fw::MouseNoButton)
                break;
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseReleaseInputEvent;
            m_inputEvent.mouseButton = button;
            m_mouseButtonStates[button] = false;
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        }
        case SDL_MOUSEWHEEL: {
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseWheelInputEvent;
            m_inputEvent.mouseButton = Fw::MouseMidButton;
            if(event.wheel.y > 0)
                m_inputEvent.wheelDirection = Fw::MouseWheelUp;
            else if(event.wheel.y < 0)
                m_inputEvent.wheelDirection = Fw::MouseWheelUp;
            else
                break;
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        }
        case SDL_FINGERDOWN: {
            Fw::MouseButton button;
            if(event.tfinger.fingerId == 0)
                button = Fw::MouseLeftButton;
            else if(event.tfinger.fingerId == 1)
                button = Fw::MouseRightButton;
            else
                break;
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseReleaseInputEvent;
            m_inputEvent.mouseButton = button;
            m_mouseButtonStates[button] = true;
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        }
        case SDL_FINGERUP: {
            Fw::MouseButton button;
            if(event.tfinger.fingerId == 0)
                button = Fw::MouseLeftButton;
            else if(event.tfinger.fingerId == 1)
                button = Fw::MouseRightButton;
            else
                break;
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseReleaseInputEvent;
            m_inputEvent.mouseButton = button;
            m_mouseButtonStates[button] = false;
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        }
        case SDL_FINGERMOTION: {
            m_inputEvent.reset();
            m_inputEvent.type = Fw::MouseMoveInputEvent;
            m_inputEvent.mouseMoved = Point(event.tfinger.dx, event.tfinger.dy);
            m_inputEvent.mousePos = Point(event.tfinger.x, event.tfinger.y);
            //g_logger.info(stdext::format("motion %d %d", event.tfinger.x, event.tfinger.y));
            if(m_onInputEvent)
                m_onInputEvent(m_inputEvent);
            break;
        }
        case SDL_QUIT:
            if(m_onClose)
                m_onClose();
            break;
        }
    }

    if(!m_maximized)
        updateUnmaximizedCoords();
}

void SDLWindow::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

void SDLWindow::showMouse()
{
    SDL_ShowCursor(1);
}

void SDLWindow::hideMouse()
{
    SDL_ShowCursor(0);
}

void SDLWindow::setMouseCursor(int cursorId)
{
    //TODO
}
void SDLWindow::restoreMouseCursor()
{
    //TODO
}

void SDLWindow::showTextInput()
{
    SDL_StartTextInput();
}

void SDLWindow::hideTextInput()
{
    SDL_StopTextInput();
}

void SDLWindow::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}

void SDLWindow::setMinimumSize(const Size& minimumSize)
{
    SDL_SetWindowMinimumSize(m_window, minimumSize.width(), minimumSize.height());
}

void SDLWindow::setFullscreen(bool fullscreen)
{
    if(m_fullscreen == fullscreen)
        return;
    SDL_SetWindowFullscreen(m_window, fullscreen);
    m_fullscreen = fullscreen;
}

void SDLWindow::setVerticalSync(bool enable)
{
    SDL_GL_SetSwapInterval(enable);
}

void SDLWindow::setIcon(const std::string& file)
{
    ImagePtr image = Image::load(file);

    if(!image) {
        g_logger.traceError(stdext::format("unable to load icon file %s", file));
        return;
    }

    if(image->getBpp() != 4) {
        g_logger.error("the app icon must have 4 channels");
        return;
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image->getPixelData(), image->getWidth(), image->getHeight(), 32, image->getWidth()*4, 0xff0000, 0xff00, 0xff, 0xff000000);
    SDL_SetWindowIcon(m_window, surface);
    SDL_FreeSurface(surface);
}

void SDLWindow::setClipboardText(const std::string& text)
{
    SDL_SetClipboardText(text.c_str());
}

Size SDLWindow::getDisplaySize()
{
    //TODO
    return getSize();
}

std::string SDLWindow::getClipboardText()
{
    return SDL_GetClipboardText();
}

std::string SDLWindow::getPlatformType()
{
    return "SDL";
}

int SDLWindow::internalLoadMouseCursor(const ImagePtr& image, const Point& hotSpot)
{
    //TODO
    return 0;
}
