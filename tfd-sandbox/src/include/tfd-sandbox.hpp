/*****************************************************************************************
 * tfd - Tophy's Flight Display                                                          *
 *       flight instruments for use in remote controls, optimized for embedded platforms *
 *                                                                                       *
 * Copyright (c) 2024 TophUwO <tophuwo01@gmail.com>                                      *
 *                                                                                       *
 * Redistribution and use in source and binary forms, with or without modification, are  *
 * permitted provided that the following conditions are met:                             *
 *  1. Redistributions of source code must retain the above copyright notice, this list  *
 *     of conditions and the following disclaimer.                                       *
 *  2. Redistributions in binary form must reproduce the above copyright notice, this    *
 *     list of conditions and the following disclaimer in the documentation and/or other *
 *     materials provided with the distribution.                                         *
 *  3. Neither the name of the copyright holder nor the names of its contributors may be *
 *     used to endorse or promote products derived from this software without specific   *
 *     prior written permission.                                                         *
 *                                                                                       *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY   *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT   *
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,        *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  *
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR    *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    *
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
 * DAMAGE.                                                                               *
 *****************************************************************************************/

/**
 * \file  tfd-sandbox.hpp
 * \brief contains class and function definitions for tfd's sandbox application
 * 
 * *tfd-sandbox* serves as not only the dedicated test application for the modules exposed by tfd itself,
 * but also as an example application that demonstrates basic as well as advanced usage of the library.
 */


#pragma once

/* Qt includes */
#include <QApplication>
#include <QMainWindow>

#include <ui_gui_mainwnd.h>

/* tfd includes */
#include <tfd/src/include/radar.hpp>


/**
 * \namespace sandbox
 * \brief     namespace containing all classes and definitions implemented by the sandbox
 *            application
 */
namespace tfd::sandbox {
    /**
     * \class MainWindow
     * \brief main window for sandbox application
     * \note  The main window automatically instantiates all flight displays needed.
     */
    class MainWindow : public QMainWindow, private Ui_MainWindow {
        Q_OBJECT
        Q_CLASSINFO("project", "tfd-sandbox")
        Q_CLASSINFO("class", "MainWindow")
        Q_CLASSINFO("brief", "main window for sandbox application")
        Q_CLASSINFO("author", "TophUwO <tophuwo01@gmail.com>")
        Q_CLASSINFO("version", "1.0.0")

    public:
        /**
         * \brief construct a new main window
         * \param [in] dim dimensions of the window (width x height) in pixels
         * \param [in] title window caption used for the new window
         */
        explicit MainWindow(QSize const &dim, QString const &title);
        ~MainWindow();

    private:
        tfd::ObjectRadar *m_radar; /**< pointer to the object radar */

        /**
         * \brief instantiate view widgets and set-up layouts and connections
         */
        void int_instantiateWidgets();
    };

    /**
     * \class SandboxApplication
     * \brief sandbox demonstrating the usage of the tfd flight instrument library
     */
    class SandboxApplication : public QApplication {
        Q_OBJECT
        Q_CLASSINFO("project", "tfd-sandbox")
        Q_CLASSINFO("class", "SandboxApplication")
        Q_CLASSINFO("brief", "sandbox demonstrating the usage of the tfd flight instrument library")
        Q_CLASSINFO("author", "TophUwO <tophuwo01@gmail.com>")
        Q_CLASSINFO("version", "1.0.0")

    public:
        /**
         * \brief create a demo application instance
         * \param [in] argc number of command-line arguments
         * \param [in] argv command-line arguments as string array
         */
        explicit SandboxApplication(int argc, char *argv[]);
        ~SandboxApplication();

        /**
         * \brief  starts the main-loop and executes the sandbox application
         * \return exit/error code
         */
        int startSandbox();

    private:
        sandbox::MainWindow *m_mainWindow; /**< pointer to the application's main window */
    };
}


