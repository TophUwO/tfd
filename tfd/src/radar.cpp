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
 * \file  radar.hpp
 * \brief implementation of the radar widget module of tfd
 *
 * tfd consists of a collection of widgets, each representing an electronic flight instrument
 * (glass cockpit) as commonly seen in commercial as well as smaller aircraft.
 */

/* stdlib includes */
#include <optional>
#include <array>
#include <chrono>
#include <iostream>

/* Qt includes */
#include <QPaintEvent>
#include <QTimer>
#include <QTimerEvent>
#include <QPainter>

/* tfd includes */
#include <tfd/src/include/radar.hpp>


namespace tfd {
    /**
     * \namespace priv
     * \brief     private namespace used for internal components
     */
    namespace priv {
        static QVariant const gl_InvVariant = QVariant(QMetaType::fromName("")); /**< invalid variant; error indicator */

        /**
         * \brief   adds an integer offset to an object type literal
         * \param   [in] first object type literal
         * \param   [in] offset value to add
         * \return  object type with the offset added
         * \warning This function does not do range checking on both input
         *          and output.
         */
        static constexpr ObjectRadar::ObjectType operator +(ObjectRadar::ObjectType first, int offset) noexcept {
            return static_cast<ObjectRadar::ObjectType>(static_cast<int>(first) + offset);
        }
        /**
         * \brief   subtracts an integer offset from an object type literal
         * \param   [in] first object type literal
         * \param   [in] offset value to subtract
         * \return  object type with the offset subtracted
         * \warning This function does not do range checking on both input
         *          and output.
         */
        static constexpr ObjectRadar::ObjectType operator -(ObjectRadar::ObjectType first, int offset) noexcept {
            return operator +(first, -offset);
        }
        static constexpr float gl_FType = static_cast<float>(ObjectRadar::ObjectType::Vehicle);   /**< lowest type index */
        static constexpr float gl_LType = static_cast<float>(ObjectRadar::ObjectType::__N__ - 1); /**< highest type index */

        /**
         * \struct __PropertyInfoEntry__
         * \brief  holds type and range information for each property entry
         */
        struct __PropertyInfoEntry__ {
            ObjectRadar::Property m_prop;  /**< property index */
            QMetaType::Type       m_type;  /**< type info for comparing types */
            std::optional<QSizeF> m_range; /**< optional range attribute for additional validation */
        };
        using PII = __PropertyInfoEntry__;
        /**
         * \brief property value type map
         *
         * This map is used for type-checking submitted values to decide whether the property
         * value can be updated.
         */
        static constexpr std::array gl_PropertyTypeLUT = {
            /* view properties */
            PII{ ObjectRadar::Property::AutoUpdate,      QMetaType::Bool                                },
            PII{ ObjectRadar::Property::UpdateRate,      QMetaType::Float,   QSizeF{ 0.1f, 240.f }      },
            PII{ ObjectRadar::Property::DefaultFont,     QMetaType::QFont,                              },
            PII{ ObjectRadar::Property::DefaultFontSize, QMetaType::Int                                 },
            PII{ ObjectRadar::Property::ForegroundColor, QMetaType::QColor                              },
            PII{ ObjectRadar::Property::BackgroundColor, QMetaType::QColor                              },
            PII{ ObjectRadar::Property::RadarCenter,     QMetaType::QPointF                             },
            PII{ ObjectRadar::Property::RadarAltitude,   QMetaType::Float                               },
            PII{ ObjectRadar::Property::RadarRange,      QMetaType::QSizeF                              },

            /* object properties */
            PII{ ObjectRadar::Property::Identifier,      QMetaType::QString                             },
            PII{ ObjectRadar::Property::Type,            QMetaType::Int,     QSizeF(gl_FType, gl_LType) },
            PII{ ObjectRadar::Property::Position,        QMetaType::QPointF                             },
            PII{ ObjectRadar::Property::Color,           QMetaType::QColor                              },
            PII{ ObjectRadar::Property::Area,            QMetaType::QSizeF                              },
            PII{ ObjectRadar::Property::Altitude,        QMetaType::Float                               },
            PII{ ObjectRadar::Property::Visibility,      QMetaType::Bool                                }
        };

        /**
         * \brief  checks whether a property index is in range of the property info map
         * \param  [in] prop property index to validate
         * \return *true* if the given property index is valid, *false* if not
         */
        static bool int_isValidPropertyIndex(ObjectRadar::Property prop) noexcept {
            auto const iprop = static_cast<size_t>(prop);
            if (iprop < 0 && iprop > gl_PropertyTypeLUT.size())
                return false;

            return true;
        }

        /**
         * \brief  carry out basic and advanced type checking for property values based on intended
         *         property type
         * 
         * The function supports type matching as well as range checking. First, the property type
         * is matched and then, if present, the range is checked.
         * 
         * \param  [in] prop property index to validate the value for
         * \param  [in] val value to check
         * \return *true* if the value type matches the requirements, *false* if not
         */
        static bool int_isValidPropertyValue(ObjectRadar::Property prop, QVariant const &val) {
            /* Check if given property index even exists. */
            if (!int_isValidPropertyIndex(prop))
                return false;

            /* Retrieve property type entry from index. */
            auto const entry = gl_PropertyTypeLUT[static_cast<size_t>(prop)];
            /* Check type. */
            if (val.typeId() ^ entry.m_type)
                return false;

            /* If range is set, validate value range, too. */
            if (entry.m_range.has_value()) {
                auto const range = entry.m_range.value();
                auto const size  = val.toFloat();
                
                /* Check if value is in range (bounds inclusive). */
                if ((size - range.width()) * (range.height() - size) < 0.f)
                    return false;
            }

            return true;
        }
    }
}


namespace tfd {
    /**
     * \class ObjectRadarPrivate
     * \brief internal state of object radar
     */
    class ObjectRadarPrivate {
        friend class ObjectRadar;

        /* widget view settings */
        bool    m_autoUpdate  = false;                            /**< automatically update view when property is changed */
        float   m_updateRate  = 30.f;                             /**< updates (redraws) per second */
        QPointF m_radarCenter = QPointF{ 0.f, 0.f };              /**< center point of the object radar, in [lat, long] */
        QSizeF  m_radarRange  = QSizeF{ 5.f, 35.f };              /**< range of the radar view in [min, max] meters */
        float   m_radarAlt    = 0.f;                              /**< altitude of the radar center, in meters above sea-level */
        QString m_defFont     = QString(":/fonts/B612_Mono.ttf"); /**< font used to display identifiers and overlays */
        int     m_defFontSize = 10;                               /**< font size for default text, identifiers, markers, etc. */
        QColor  m_fgndColor   = QColor(Qt::gray);                 /**< color used for text and indicators */
        QColor  m_bgndColor   = QColor(Qt::black);                /**< color used for backgrounds and surface fills */

        /* utilities */
        QTimer m_redrawTimer; /**< widget redraw timer (m_updateRate hz period) */
    };


    ObjectRadar::ObjectRadar(QSize const &dim, QWidget *parent)
        : QWidget(parent), m_data(std::make_unique<ObjectRadarPrivate>())
    {
        /* Setup widget. */
        setFixedSize(dim);
        setCursor(Qt::CursorShape::BlankCursor);

        /* Setup repaint timer. */
        connect(&m_data->m_redrawTimer, &QTimer::timeout, this, [&]() {
            /*
             * Issue repaint immediately. In this situation, we do not want to queue a repaint via
             * update() even though it would probably not make a big difference in practice since
             * repaint messages are high-priority.
             */
            repaint();
        });
        m_data->m_redrawTimer.setTimerType(Qt::TimerType::PreciseTimer);
        m_data->m_redrawTimer.setInterval(std::chrono::duration<float, std::milli>(1000.f / m_data->m_updateRate).count());
        m_data->m_redrawTimer.start();
    }

    ObjectRadar::~ObjectRadar() {
        m_data->m_redrawTimer.stop();
    }

    bool ObjectRadar::addObject(QString const &ident, ObjectRadar::ObjectType type, QPointF const &pos, float alt) {
        return false;
    }

    bool ObjectRadar::removeObject(QString const &ident) {
        return false;
    }

    void ObjectRadar::removeAllObjects() {

    }

    bool ObjectRadar::hasObject(QString const &ident) const noexcept {
        return false;
    }

    QVariant ObjectRadar::getProperty(ObjectRadar::Property prop) const noexcept {
        /* Check if property index exists. */
        if (!priv::int_isValidPropertyIndex(prop))
            return priv::gl_InvVariant;

        switch (prop) {
            case ObjectRadar::Property::AutoUpdate:      return m_data->m_autoUpdate;
            case ObjectRadar::Property::UpdateRate:      return m_data->m_updateRate;
            case ObjectRadar::Property::DefaultFont:     return m_data->m_defFont;
            case ObjectRadar::Property::DefaultFontSize: return m_data->m_defFontSize;
            case ObjectRadar::Property::ForegroundColor: return m_data->m_fgndColor;
            case ObjectRadar::Property::BackgroundColor: return m_data->m_bgndColor;
            case ObjectRadar::Property::RadarCenter:     return m_data->m_radarCenter;
            case ObjectRadar::Property::RadarAltitude:   return m_data->m_radarAlt;
            case ObjectRadar::Property::RadarRange:      return m_data->m_radarRange;
        }

        return priv::gl_InvVariant;
    }

    QVariant ObjectRadar::getProperty(QString const &ident, ObjectRadar::Property prop) const noexcept {
        return priv::gl_InvVariant;
    }

    bool ObjectRadar::setProperty(ObjectRadar::Property prop, QVariant const &val) {
        return false;
    }

    bool ObjectRadar::setProperty(QString const &ident, ObjectRadar::Property prop, QVariant const &val) {
        return false;
    }

    void ObjectRadar::paintEvent(QPaintEvent *pe) {
        /* Setup painter. */
        QPainter painter(this);

        /* Fill background. */
        painter.fillRect(QRect{ 0, 0, width(), height() }, m_data->m_bgndColor);
    }
}


