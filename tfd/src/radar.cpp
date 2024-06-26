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
#include <unordered_map>

/* Qt includes */
#include <QPaintEvent>
#include <QTimer>
#include <QTimerEvent>
#include <QPainter>
#include <QTest>

/* tfd includes */
#include <tfd/src/include/radar.hpp>


namespace tfd {
    /**
     * \namespace priv
     * \brief     private namespace used for internal components
     */
    /* property system */
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
        static constexpr float gl_FType   = static_cast<float>(ObjectRadar::ObjectType::Vehicle);   /**< lowest type index */
        static constexpr float gl_LType   = static_cast<float>(ObjectRadar::ObjectType::__N__ - 1); /**< highest type index */
        static constexpr float gl_FPStyle = static_cast<float>(Qt::NoPen);                          /**< first valid outline style */
        static constexpr float gl_LPStyle = static_cast<float>(Qt::DashDotDotLine);                 /**< last valid outline style */

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
            PII{ ObjectRadar::Property::UpdateRate,      QMetaType::Float,   QSizeF{ 0.05f, 240.f }           },
            PII{ ObjectRadar::Property::StaticTextFont,  gl_FPType                                            },
            PII{ ObjectRadar::Property::LabelFont,       gl_FPType                                            },
            PII{ ObjectRadar::Property::ObjectLabelFont, gl_FPType                                            },
            PII{ ObjectRadar::Property::ForegroundColor, QMetaType::QColor                                    },
            PII{ ObjectRadar::Property::BackgroundColor, QMetaType::QColor                                    },
            PII{ ObjectRadar::Property::RadarCenter,     QMetaType::QPointF                                   },
            PII{ ObjectRadar::Property::RadarAltitude,   QMetaType::Float                                     },
            PII{ ObjectRadar::Property::RadarRange,      QMetaType::QSizeF                                    },
            PII{ ObjectRadar::Property::AreaOpacity,     QMetaType::Int,     QSizeF{ 0.f, 255.f }             },
            PII{ ObjectRadar::Property::OutlineStrength, QMetaType::Int,     QSizeF{ 0.f, 20.f }              },
            PII{ ObjectRadar::Property::OutlineStyle,    QMetaType::Int,     QSizeF{ gl_FPStyle, gl_LPStyle } },

            /* object properties */
            PII{ ObjectRadar::Property::Identifier,      QMetaType::QString                                   },
            PII{ ObjectRadar::Property::Type,            QMetaType::Int,     QSizeF{gl_FType, gl_LType}       },
            PII{ ObjectRadar::Property::Position,        QMetaType::QPointF                                   },
            PII{ ObjectRadar::Property::Color,           QMetaType::QColor                                    },
            PII{ ObjectRadar::Property::Area,            gl_PAType                                            },
            PII{ ObjectRadar::Property::Altitude,        QMetaType::Float                                     },
            PII{ ObjectRadar::Property::Visibility,      QMetaType::Bool                                      }
        };

        /**
         * \brief  checks whether a property index is in range of the property info map
         * \param  [in] prop property index to validate
         * \return *true* if the given property index is valid, *false* if not
         */
        static bool int_isValidPropertyIndex(ObjectRadar::Property prop) noexcept {
            auto const iprop = static_cast<size_t>(prop);
            if (iprop < 0 || iprop >= gl_PropertyTypeLUT.size())
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


    /* radar object manager */
    namespace priv {
        /**
         * \class RadarObject
         * \brief represents an object of a specific type, visible on the radar screen
         */
        struct RadarObject {
            /**
             * \brief constructs a new radar object
             * \param [in] type type identifier of the new object
             * \note  All other fields of the RadarObject type are initialized
             *        to sensible default values.
             */
            explicit RadarObject(ObjectRadar::ObjectType type)
                : m_type(type), m_isVisible(true), m_altitude(0.f)
            { }
            /**
             * \brief make a new radar object
             * \param [in] type type of the new object
             * \param [in] pos initial [lat, long] position of object
             * \param [in] alt initial altitude in meters above sea-level
             * \note  Radar objects are visible by default.
             */
            explicit RadarObject(ObjectRadar::ObjectType type, QPointF const &pos, float alt)
                : m_type(type), m_position(pos), m_altitude(alt), m_isVisible(true)
            { }

            ObjectRadar::ObjectType m_type;      /**< object type ID */
            QPointF                 m_position;  /**< [lat, long] position */
            QColor                  m_color;     /**< color of indicator and identifier */
            QSizeF                  m_area;      /**< area size (only valid when m_type == Area) */
            float                   m_altitude;  /**< altitude in meters above sea-level */
            bool                    m_isVisible; /**< whether or not the object is visible or hidden */
        };
        using RO = RadarObject;

        /**
         * \class RadarObjectManager
         * \brief manages all radar present radar objects
         */
        class RadarObjectManager : public QObject {
            Q_OBJECT
            
        public:
            friend class tests::ObjectRadarTests;

            /**
             * \brief  adds an object to the radar (if there is no object with the same identifier)
             * \param  [in] ident identifier of the radar object that is to be added
             * \param  [in] obj radar object properties
             * \return *true* if the element could be added, *false* if not
             * \note   This function never throws exceptions.
             */
            bool addObject(QString const &ident, priv::RadarObject const &obj) noexcept {
                try {
                    /* Check if object with the same identifier already exists. If yes, abort. */
                    if (m_objMap.find(ident) != m_objMap.end())
                        return false;

                    /* Add object entry. */
                    bool const res = m_objMap.insert({ ident, obj }).second;

                    if (res)
                        emit objectAdded(ident);
                    return res;
                } catch (...) { }

                return false;
            }
            /**
             * \brief  removes an object from the radar, identified by their name
             * \param  [in] ident (unique) identifier of the object that is to be removed
             * \return *true* if the object was removed, *false* if not
             * \note   This function never throws exceptions.
             */
            bool removeObject(QString const &ident) noexcept {
                bool const res = m_objMap.erase(ident) == 1;

                if (res)
                    emit objectRemoved(ident);
                return res;
            }
            /**
             * \brief removes all radar objects
             * \note  This function never throws exceptions.
             */
            void clearObjects() noexcept {
                m_objMap.clear();

                emit objectRemoved({});
            }
            /**
             * \brief  retrieves a pointer to an object with a given name
             * \param  [in] ident (unique) identifier of the object that is to be retrieved
             * \return an *std::optional* with the pointer of the retrieved object if it could
             *         be retrieved, or an empty optional if there was an error or the object
             *         could not be retrieved
             * \note   This function never throws exceptions.
             */
            std::optional<priv::RadarObject *> getObject(QString const &ident) noexcept {
                /* Get iterator for requested object. */
                auto it = m_objMap.find(ident);
                /* If the object could not be retrieved, return an empty optional. */
                if (it == m_objMap.end())
                    return std::optional<priv::RadarObject *>{};

                /* Return optional with pointer to retrieved object. */
                return std::optional<priv::RadarObject *>(&((*it).second));
            }

        signals:
            /**
             * \brief emitted when an object was successfully added
             * \param [in] ident identifier of the object that was added
             */
            void objectAdded(QString const &ident);
            /**
             * \brief emitted when an (or all) object was successfully removed
             * \param [in] ident identifier of removed object, or empty if all objects were removed
             * \note  The *std::optional* parameter is empty if all objects were removed, otherwise
             *        it holds the name of the removed object as a *QString*.
             */
            void objectRemoved(std::optional<QString> const &ident);

        private:
            std::unordered_map<QString, priv::RO> m_objMap; /**< radar object container */
        };
        using ROM = RadarObjectManager;
    }


    /* miscellaneous internal functions used by the radar */
    namespace priv {
        /**
         * \brief tries to (re-)initialize internal repaint timer
         *
         * When the widget is constructed, the repaint timer is initialized with the initial
         * (default) update-rate (60hz). When the update rate is changed in response to a call
         * to ObjectRadar::setProperty(), this function is called again on the running timer
         * to make sure it's updated properly. Using this simplistic approach, depending on the
         * ratio between the old and the new update rate, there might be a small lag between the
         * last frame ratio using the old update rate and the first frame using the new update
         * rate.
         * 
         * \param [in] updpersec new updates per second, in 1/f seconds
         * \param [out] timer pointer to timer object to modify
         * \note  If **timer** is *nullptr* or the update interval of the timer is the same as
         *        the new update interval, the function does nothing.
         */
        static void int_tryInitializeRepaintTimer(float const updpersec, QTimer *timer) noexcept {
            /* Calculate new interval. */
            auto const ninterval = std::chrono::duration<float, std::milli>(1000.f / updpersec).count();
            /*
             * If the timer object is invalid or the new interval equals to the new interval,
             * do nothing.
             */
            if (timer == nullptr || timer->interval() == ninterval)
                return;

            /* Update interval and restart. */
            timer->stop();
            timer->setInterval(ninterval);
            timer->start();
        }

        /**
         * \brief
         */
        static bool int_drawCompass(ObjectRadarPrivate *data, QPixmap &target) noexcept {
            // TODO: document
            // TODO: implement
        }
    }
}


namespace tfd {
    class tests::ObjectRadarTests;

    /**
     * \class ObjectRadarPrivate
     * \brief internal state of object radar
     */
    class ObjectRadarPrivate : public QObject {
        Q_OBJECT

    public slots:
        /**
         * \brief updates the internal cache that is used to store prepared display
         *        resources and computed but rarely changed computation results
         * 
         * This function is usually not called by the user or the programmar explicitly.
         * It's intended to be used as a slot. The function decides what resources to
         * update based on the parameters of the signal.
         * 
         * \param [in] prop index of the property that was updated
         * \param [in] val new value of the property
         */
        void updateCache(ObjectRadar::Property prop, QVariant const &val) {
            /* Whether or not to update (= (re-)initialize) entire cache. */
            bool const isall = prop == static_cast<ObjectRadar::Property>(INT_MAX);
            
            /* Update what needs to be updated. */
            switch (prop) {
                // TODO: implement
            }
        }
        /**
         */
        void updateTrackedObject(std::optional<QString> const &ident) {

        }

    private:
        friend class ObjectRadar;
        friend class tests::ObjectRadarTests;

        /* widget view settings */
        float     m_updateRate      = 30.f;                              /**< updates (redraws) per second */
        QPointF   m_radarCenter     = QPointF{ 0.f, 0.f };               /**< center point of the object radar, in [lat, long] */
        QSizeF    m_radarRange      = QSizeF{ 5.f, 35.f };               /**< range of the radar view in [min, max] meters */
        float     m_radarAlt        = 0.f;                               /**< altitude of the radar center, in meters above sea-level */
        FP        m_staticTextFont  = FP{ ":/fonts/B612_Mono.ttf", 10 }; /**< properties for the static font INSIDE the radar view */
        FP        m_labelFont       = FP{ ":/fonts/B612_Mono.ttf", 11 }; /**< properties for label font OUTSIDE the radar view */
        FP        m_objLabelFont    = FP{ ":/fonts/B612_Mono.ttf", 9 };  /**< properties of the font used for object labels INSIDE the radar view */
        QColor    m_fgndColor       = QColor(Qt::gray);                  /**< color used for text and indicators */
        QColor    m_bgndColor       = QColor(Qt::black);                 /**< color used for backgrounds and surface fills */
        int       m_areaOpacity     = 0.4f * 255;                        /**< opacity used for fill colors, in range [0 ... 255] */
        int       m_outlineStrength = 2;                                 /**< width of area and path outlines, in pixels */
        int       m_outlineStyle    = Qt::SolidLine;                     /**< style of path and area outline, one value of the *Qt::PenStyle* enum */
        priv::RO *m_trackedObject   = nullptr;                           /**< currently tracked radar object or *nullptr* if no object is being tracked */

        /* utilities */
        QTimer    m_redrawTimer; /**< widget redraw timer (m_updateRate hz period) */
        priv::ROM m_objManager;  /**< radar object manager */

        /* cached resources */
        QPixmap m_c_radarCompass;         /**< pre-rendered image of the compass */
        QPixmap m_c_radarScale;           /**< pre-rendered image of the radar scale (circles around center, etc.) */
        QFont   m_c_radarStaticTextFont;  /**< cached font for all static text that is WITHIN the radar view */
        QFont   m_c_radarLabelFont;       /**< cached font used for labels OUTSIDE the radar view */
        QFont   m_c_radarObjectLabelFont; /**< cached font used for object labels INSIDE the radar view */
    };


    ObjectRadar::ObjectRadar(QSize const &dim, QWidget *parent)
        : QWidget(parent), m_data(std::make_unique<ObjectRadarPrivate>())
    {
        /* Setup widget. */
        setFixedSize(dim);
        setCursor(Qt::CursorShape::BlankCursor);

        /* Initialize cached resources. */
        m_data->updateCache(static_cast<ObjectRadar::Property>(INT_MAX), {});
        connect(this, &ObjectRadar::propertyValueChanged, m_data.get(), &ObjectRadarPrivate::updateCache);
        connect(&m_data->m_objManager, &priv::ROM::objectAdded, m_data.get(), &ObjectRadarPrivate::updateTrackedObject);
        connect(&m_data->m_objManager, &priv::ROM::objectRemoved, m_data.get(), &ObjectRadarPrivate::updateTrackedObject);

        /* Setup repaint timer. */
        connect(&m_data->m_redrawTimer, &QTimer::timeout, this, [&]() {
            /*
             * Issue repaint immediately. In this situation, we do not want to queue a repaint via
             * *QWidget::update()* even though it would probably not make a big difference in practice since
             * repaint messages are high-priority.
             */
            repaint();
        });
        m_data->m_redrawTimer.setTimerType(Qt::TimerType::PreciseTimer);
        priv::int_tryInitializeRepaintTimer(m_data->m_updateRate, &m_data->m_redrawTimer);
    }

    ObjectRadar::~ObjectRadar() {
        m_data->m_redrawTimer.stop();
    }

    bool ObjectRadar::addObject(QString const &ident, ObjectRadar::ObjectType type, QPointF const &pos, float alt) {
        if (type < static_cast<ObjectRadar::ObjectType>(0) || type >= ObjectRadar::ObjectType::__N__)
            return false;

        return m_data->m_objManager.addObject(
            ident,
            priv::RadarObject{ type, pos, alt }
        );
    }

    bool ObjectRadar::removeObject(QString const &ident) {
        return m_data->m_objManager.removeObject(ident);
    }

    void ObjectRadar::removeAllObjects() {
        m_data->m_objManager.clearObjects();
    }

    bool ObjectRadar::hasObject(QString const &ident) const noexcept {
        return m_data->m_objManager.getObject(ident).has_value();
    }

    QVariant ObjectRadar::getProperty(ObjectRadar::Property prop) const noexcept {
        /* Check if property index exists. */
        if (!priv::int_isValidPropertyIndex(prop))
            return priv::gl_InvVariant;

        /* Select the desired property. */
        switch (prop) {
            case ObjectRadar::Property::UpdateRate:      return m_data->m_updateRate;
            case ObjectRadar::Property::ForegroundColor: return m_data->m_fgndColor;
            case ObjectRadar::Property::BackgroundColor: return m_data->m_bgndColor;
            case ObjectRadar::Property::RadarCenter:     return m_data->m_radarCenter;
            case ObjectRadar::Property::RadarAltitude:   return m_data->m_radarAlt;
            case ObjectRadar::Property::RadarRange:      return m_data->m_radarRange;
        }

        /* If the property could not be retrieved, return an invalid variant. */
        return priv::gl_InvVariant;
    }

    QVariant ObjectRadar::getProperty(QString const &ident, ObjectRadar::Property prop) const noexcept {
        /* Get object. */
        priv::RadarObject const *robj = m_data->m_objManager.getObject(ident).value_or(nullptr);
        if (robj == nullptr)
            return priv::gl_InvVariant;

        /* Select property. */
        switch (prop) {
            case ObjectRadar::Property::Identifier: return ident;
            case ObjectRadar::Property::Type:       return static_cast<size_t>(robj->m_type);
            case ObjectRadar::Property::Position:   return robj->m_position;
            case ObjectRadar::Property::Color:      return robj->m_color;
            case ObjectRadar::Property::Area:       return robj->m_area;
            case ObjectRadar::Property::Altitude:   return robj->m_altitude;
            case ObjectRadar::Property::Visibility: return robj->m_isVisible;
        }

        /* If the property could not be retrieved, return an invalid variant. */
        return priv::gl_InvVariant;
    }

    bool ObjectRadar::setProperty(ObjectRadar::Property prop, QVariant const &val) {
        /* Check if property type exists and the type is correct. */
        if (!priv::int_isValidPropertyValue(prop, val))
            return false;

        /* Update selected view property. */
        switch (prop) {
            case ObjectRadar::Property::StaticTextFont:  m_data->m_staticTextFont  = val.value<FontProperties>(); break;
            case ObjectRadar::Property::LabelFont:       m_data->m_labelFont       = val.value<FontProperties>(); break;
            case ObjectRadar::Property::ObjectLabelFont: m_data->m_objLabelFont    = val.value<FontProperties>(); break;
            case ObjectRadar::Property::ForegroundColor: m_data->m_fgndColor       = val.value<QColor>();         break;
            case ObjectRadar::Property::BackgroundColor: m_data->m_bgndColor       = val.value<QColor>();         break;
            case ObjectRadar::Property::RadarCenter:     m_data->m_radarCenter     = val.toPointF();              break;
            case ObjectRadar::Property::RadarAltitude:   m_data->m_radarAlt        = val.toFloat();               break;
            case ObjectRadar::Property::UpdateRate:      m_data->m_updateRate      = val.toFloat();               break;
            case ObjectRadar::Property::RadarRange:      m_data->m_radarRange      = val.toSizeF();               break;
            case ObjectRadar::Property::AreaOpacity:     m_data->m_areaOpacity     = val.toInt();                 break;
            case ObjectRadar::Property::OutlineStrength: m_data->m_outlineStrength = val.toInt();                 break;
            case ObjectRadar::Property::OutlineStyle:    m_data->m_outlineStyle    = val.toInt();                 break;
            default:
                /* Provided invalid property index. */
                return false;
        }

        emit propertyValueChanged(prop, val);
        return true;
    }

    bool ObjectRadar::setProperty(QString const &ident, ObjectRadar::Property prop, QVariant const &val) {
        /* Check if property type exists and the type is correct. */
        if (!priv::int_isValidPropertyValue(prop, val))
            return false;
        /* Get object. */
        priv::RadarObject *obj = m_data->m_objManager.getObject(ident).value_or(nullptr);
        if (obj == nullptr)
            return false;

        /* Update property. */
        switch (prop) {
            case ObjectRadar::Property::Identifier:
                /* Add object with duplicate properties under new name. */
                if (!m_data->m_objManager.addObject(val.toString(), *obj))
                    return false;

                /* Delete old property. */
                return m_data->m_objManager.removeObject(ident);
            case ObjectRadar::Property::Type:
                obj->m_type = static_cast<ObjectRadar::ObjectType>(val.toInt());
                
                return true;
            case ObjectRadar::Property::Position:   obj->m_position = val.toPointF();       return true;
            case ObjectRadar::Property::Color:      obj->m_color     = val.value<QColor>(); return true;
            case ObjectRadar::Property::Area:       obj->m_area      = val.toSizeF();       return true;
            case ObjectRadar::Property::Altitude:   obj->m_altitude  = val.toFloat();       return true;
            case ObjectRadar::Property::Visibility: obj->m_isVisible = val.toBool();        return true;
        }

        return false;
    }

    std::optional<QString> const ObjectRadar::getTrackedObject() const noexcept {
        if (m_data->m_trackedObject == nullptr)
            return std::optional<QString>{};

        // TODO implement
        return std::optional<QString>{};
    }

    void ObjectRadar::setTrackedObject(QString const &ident) noexcept {
        /* Try finding the referenced object. */
        priv::RadarObject *ro = m_data->m_objManager.getObject(ident).value_or(nullptr);
        if (ro == nullptr)
            return;

        /* Update tracked object cache. */
        m_data->m_trackedObject = ro;
    }

    void ObjectRadar::paintEvent(QPaintEvent *pe) {
        /* Setup painter. */
        QPainter painter(this);

        /* Fill background. */
        painter.fillRect(QRect{ 0, 0, width(), height() }, m_data->m_bgndColor);
    }
}


/* polygon area */
namespace tfd {
    bool RadarArea::addVertex(QPointF const &vertex) noexcept {
        try {
            m_vertices.push_back(vertex);
        } catch (...) { return false; }

        return true;
    }

    bool RadarArea::removeVertex(int index) noexcept {
        if (m_vertices.size() >= index || index < 0)
            return false;

        m_vertices.erase(m_vertices.begin() + index);
        return true;
    }

    void RadarArea::clearVertices() noexcept {
        m_vertices.clear();
    }
}


/* unit tests for object radar */
namespace tfd {
    /**
     * \namespace tests
     * \brief     contains all unit tests employed by tfd
     */
    namespace tests {
        /**
         * \class ObjectRadarTests
         * \brief defines unit tests for the ObjectRadarTests class
         * \note  Do not directly instantiate this class in user code. Use
         *        the RunObjectRadarTests function instead in order to invoke
         *        the unit tests contained within this class.
         */
        class ObjectRadarTests : public QObject {
            Q_OBJECT

        private:
            tfd::ObjectRadar m_radar{ QSize{ 600, 600 }, nullptr }; /**< object radar instance */

        private slots:
            /**
             * \brief runs before each test function is invoked 
             */
            void init() {
                m_radar.m_data->m_objManager.m_objMap.clear();
            }

            /**
             * \brief tests whether the view property retrieval works 
             */
            void testObjectRadarGetViewPropertiesValid() {
                /* Test with valid property. */
                QVERIFY(m_radar.getProperty(ObjectRadar::Property::ForegroundColor).isValid());
                /* Test with invalid property. */
                QVERIFY(!m_radar.getProperty(static_cast<ObjectRadar::Property>(INT_MAX)).isValid());
                /* Test with negative property index. */
                QVERIFY(!m_radar.getProperty(static_cast<ObjectRadar::Property>(-1)).isValid());
                /* Test with count index (also out of range). */
                QVERIFY(!m_radar.getProperty(ObjectRadar::Property::__N__).isValid());
            }
            /**
             * \brief tests whether a radar object can be instantiated and verifies its initial properties 
             */
            void testObjectRadarCreateObject() {
                /* Create test radar object. */
                tfd::priv::RadarObject obj{ ObjectRadar::ObjectType::Vehicle, QPointF{ -12.f, 178.f }, 576.f };

                /* Verify properties of test radar object. */
                QVERIFY(obj.m_type == ObjectRadar::ObjectType::Vehicle);
                QVERIFY((obj.m_position == QPointF{ -12.f, 178.f }));
                QVERIFY(obj.m_altitude == 576.f);
            }
            /**
             * \brief simulates adding a well-formed radar object as well as subsequent retrieval 
             */
            void testObjectRadarAddAndRetrieveObjectSuccessful() {
                /* Add object to radar. */
                QVERIFY(m_radar.addObject("testObject", ObjectRadar::ObjectType::Vehicle, QPointF{ 0.f, 0.f }));
                /* Check side-effects. */
                QVERIFY(m_radar.m_data->m_objManager.m_objMap.size() == 1);

                /* Check if object can be retrieved. */
                auto const obj = m_radar.m_data->m_objManager.getObject("testObject");
                QVERIFY(obj.has_value());
                QVERIFY(obj.value()->m_type == ObjectRadar::ObjectType::Vehicle);
            }
            /**
             * \brief simulates adding an ill-formed radar object and tests whether the object actually
             *        got rejected by the internal object manager
             */
            void testObjectRadarAddAndRetrieveObjectFailed() {
                /* Try adding an object with invalid parameters. */
                QVERIFY(!m_radar.addObject("testObject", ObjectRadar::ObjectType::__N__, QPointF{ 0.f, 0.f }));
                /* Check side-effects. */
                QVERIFY(m_radar.m_data->m_objManager.m_objMap.size() == 0);

                /* Check if object can be retrieved. */
                auto const obj = m_radar.m_data->m_objManager.getObject("testObject");
                QVERIFY(!obj.has_value());
            }
            /**
             * \brief tests whether object properties can be obtained for existing and non-existing radar
             *        objects 
             */
            void testObjectRadarGetObjectProperties() {
                /* Add a few test radar objects. */
                QVERIFY(m_radar.addObject("testObject1", ObjectRadar::ObjectType::Vehicle, QPointF{ 1.f, 1.f }));
                QVERIFY(m_radar.addObject("testObject2", ObjectRadar::ObjectType::Area, QPointF{ 65.f, -12.f }));
                QVERIFY(m_radar.addObject("testObject3", ObjectRadar::ObjectType::Path, QPointF{ 5.f, -100.f }));
                QVERIFY(m_radar.addObject("testObject4", ObjectRadar::ObjectType::Person, QPointF{ 19.f, 89.f }));

                /* Retrieve properties of existing objects. */
                QVERIFY((m_radar.getProperty("testObject2", ObjectRadar::Property::Position) == QPointF{ 65.f, -12.f }));
                QVERIFY(m_radar.getProperty("testObject3", ObjectRadar::Property::Type) == static_cast<int>(ObjectRadar::ObjectType::Path));
                QVERIFY(m_radar.getProperty("testObject1", ObjectRadar::Property::Altitude) == 0.f);

                /* Try retrieving properties of non-existent objects. */
                QVERIFY(!m_radar.getProperty("testObject111", ObjectRadar::Property::Color).isValid());
                /* Try retrieving a non-existent property. */
                QVERIFY(!m_radar.getProperty("testObject2", ObjectRadar::Property::__N__).isValid());
            }
            /**
             * \brief tests whether view properties can be updated 
             */
            void testObjectRadarSetViewProperty() {
                /* Verify that the view property is in its initial state. */
                QVERIFY((m_radar.getProperty(ObjectRadar::Property::ForegroundColor) == QColor{ Qt::gray }));
                /* Update the property. */
                QVERIFY((m_radar.setProperty(ObjectRadar::Property::ForegroundColor, QColor{ 255, 70, 70 })));
                /* Verify that the property was indeed updated. */
                QVERIFY((m_radar.getProperty(ObjectRadar::Property::ForegroundColor) == QColor{ 255, 70, 70 }));

                /* Try modifying an non-existent property. */
                QVERIFY(!m_radar.setProperty(static_cast<ObjectRadar::Property>(1000), "value"));
                /*
                 * Try modifying an existing property that is actually associated with objects rather
                 * than the view itself.
                 */
                QVERIFY(!m_radar.setProperty(ObjectRadar::Property::Area, QSizeF{ 12.f, 12.f }));
            }
            /**
             * \brief tests whether object properties can be updated
             */
            void testObjectSetRadarObjectProperty() {
                /* Add a few test radar objects. */
                QVERIFY(m_radar.addObject("testObject1", ObjectRadar::ObjectType::Vehicle, QPointF{ 1.f, 1.f }));
                QVERIFY(m_radar.addObject("testObject2", ObjectRadar::ObjectType::Area, QPointF{ 65.f, -12.f }));

                /* Update a radar property of one object. */
                QVERIFY((m_radar.setProperty("testObject1", ObjectRadar::Property::Position, QPointF{ -40.f, -21.f })));
                /* Verify it was indeed updated. */
                QVERIFY((m_radar.getProperty("testObject1", ObjectRadar::Property::Position) == QPointF{ -40.f, -21.f }));
                /* Verify that the other object's property is still the initial value. */
                QVERIFY((m_radar.getProperty("testObject2", ObjectRadar::Property::Position) == QPointF{ 65.f, -12.f }));

                /* Try updating a non-existent property. */
                QVERIFY(!(m_radar.setProperty("testObject1", static_cast<ObjectRadar::Property>(1000), "non-existent_property")));
                /* Try updating a property for a non-existent radar object. */
                QVERIFY(!(m_radar.setProperty("non_existent_radar_object", ObjectRadar::Property::Position, QPointF{ -40.f, -21.f })));
                /* Try updating a property with an invalid value. */
                QVERIFY(!(m_radar.setProperty("testObject1", ObjectRadar::Property::Position, QColor{ Qt::magenta })));
            }
            /**
             */
            void testUpdateIdentifierOfRadarObject() {
                /* Add a few test radar objects. */
                QVERIFY(m_radar.addObject("testObject1", ObjectRadar::ObjectType::Vehicle, QPointF{ 1.f, 1.f }));
                QVERIFY(m_radar.addObject("testObject2", ObjectRadar::ObjectType::Area, QPointF{ 65.f, -12.f }));

                /* Update the name of the first to a name different from the name of the second. */
                QVERIFY(m_radar.setProperty("testObject1", ObjectRadar::Property::Identifier, "newName"));
                /* Verify the object is queriable under the new name. */
                QVERIFY(m_radar.m_data->m_objManager.getObject("newName").has_value());

                /* Try updating name to the name of an already existing object. */
                QVERIFY(!m_radar.setProperty("newName", ObjectRadar::Property::Identifier, "testObject2"));
            }
            /**
             * \brief whether finding and removing an object from the radar works 
             */
            void testObjectRadarRemoveObject() {
                /* Add some objects again. */
                QVERIFY(m_radar.addObject("testObject1", ObjectRadar::ObjectType::Vehicle, QPointF{ 1.f, 1.f }));
                QVERIFY(m_radar.addObject("testObject2", ObjectRadar::ObjectType::Area, QPointF{ 65.f, -12.f }));

                /*
                 * Remove one, verify size and that the last remaining object is the one
                 * not removed.
                 */
                QVERIFY(m_radar.removeObject("testObject1"));
                QVERIFY(m_radar.m_data->m_objManager.m_objMap.size() == 1);
                QVERIFY(m_radar.m_data->m_objManager.getObject("testObject2").has_value());
            }
            /**
             * \brief tests whether all objects can be removed at once 
             */
            void testObjectRadarRemoveAllObjects() {
                /* Add some objects again. */
                QVERIFY(m_radar.addObject("testObject1", ObjectRadar::ObjectType::Vehicle, QPointF{ 1.f, 1.f }));
                QVERIFY(m_radar.addObject("testObject2", ObjectRadar::ObjectType::Area, QPointF{ 65.f, -12.f }));
                QVERIFY(m_radar.addObject("testObject3", ObjectRadar::ObjectType::Path, QPointF{ 5.f, -100.f }));
                QVERIFY(m_radar.addObject("testObject4", ObjectRadar::ObjectType::Person, QPointF{ 19.f, 89.f }));
                /* Verify size. */
                QVERIFY(m_radar.m_data->m_objManager.m_objMap.size() == 4);

                /* Delete all objects and verify size again. */
                m_radar.removeAllObjects();
                QVERIFY(m_radar.m_data->m_objManager.m_objMap.size() == 0);
            }
            /**
             * \brief tests whether the view can track an object and behave accordingly 
             */
            void testTrackedRadarObject() {
                // TODO: implement
            }
            /**
             * \brief tests the functionality of the RadarArea class
             */
            void testRadarArea() {
                // TODO: implement
            }
            /**
             * \brief tests the functionality of the RadarPath class
             */
            void testRadarPath() {
                // TODO: implement
            }
        };
    }

    int RunObjectRadarTests() {
        auto tests = tests::ObjectRadarTests();

        /* Run object radar tests. */
        return QTest::qExec(&tests);
    }
}


/*
 * Include this file to fix linker errors that arise when defining a class
 * inside a .cpp file that carries the Q_OBJECT macro.
 */
#include <radar.moc>


