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
 * \file  tfd.hpp
 * \brief base declarations and symbols used by the entire tfd library
 * 
 * tfd consists of a collection of widgets, each representing an electronic flight instrument
 * as commonly seen in commercial as well as smaller aircraft.
 */

#pragma once

#if (defined _MSC_VER)
    #define TFD_EXTERN extern
    
    #if (defined __TFD_BUILD_SO__)
        #define TFD_API __declspec(dllexport)
    #elif (defined __TFD_USE_SO__)
        #define TFD_API __declspec(dllimport)
    #else
        #error Incorrect project or build configuration. Check build settings.
    #endif
#else
    #error At this moment, only the Microsoft Visual C++ compiler (MSVC) is supported.
#endif


/* Check some basic requirements. */
static_assert(sizeof(char) == 1, "sizeof(char) must be exactly one byte!");
static_assert(sizeof(int) == 4, "sizeof(int) must be exactly four bytes!");
static_assert(sizeof(float) == 4, "sizeof(float) must be exactly four bytes!");



/**
 * \namespace tfd
 * \brief     holds all classes and functionality exposed by the tfd library
 */
namespace tfd {
    /**
     * \class FontProperties
     * \brief holds properties for display fonts used by tfd widgets
     */
    struct FontProperties {
        FontProperties()                            = default;
        FontProperties(FontProperties const &props) = default;
        ~FontProperties()                           = default;
        /**
         * \brief construct a new FontProperties object
         * \param [in] fam font family (nearest match)
         * \param [in] pt size of the new font
         * \param [in] weight boldness of the new font (100, 200, ..., 600) {def: -1}
         * \param [in] italic whether or not the font should be *cursive* (*italic*) {def: false} 
         */
        explicit FontProperties(QString const &fam, int pt, int weight = -1, bool italic = false)
            : m_family(fam), m_pointSize(pt), m_weight(weight), m_isItalic(italic)
        { }

        QString m_family    = ":/fonts/B612_Mono.ttf"; /**< name of font family */
        int     m_pointSize = -1;                      /**< font size, in pt */
        int     m_weight    = -1;                      /**< font weight (regular, bold, heavy, black, ...) */
        bool    m_isItalic  = false;                   /**< whether or not the font will appear cursive or not */
    };
    Q_DECLARE_METATYPE(FontProperties);

    /**
     * \brief introduce alias as a short form    
     */
    using FP = FontProperties;

    /**
     * \defgroup custom type IDs 
     * \brief    defines type ID for classes and data-structures that are to be used as values (used internally
     *           in the property system for type checking, etc.)
     */
    constexpr inline QMetaType::Type gl_FPType = static_cast<QMetaType::Type>(QMetaType::Type::User + 1);
    constexpr inline QMetaType::Type gl_PAType = static_cast<QMetaType::Type>(QMetaType::Type::User + 2);
}


