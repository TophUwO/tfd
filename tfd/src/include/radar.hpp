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
 * \brief definitions of the radar widget module of tfd
 *
 * tfd consists of a collection of widgets, each representing an electronic flight instrument
 * (glass cockpit) as commonly seen in commercial as well as smaller aircraft.
 */

#pragma once

/* Qt includes */
#include <QWidget>
#include <QVariant>

/* tfd includes */
#include <tfd/src/include/tfd.hpp>


/**
 * \namespace tfd
 * \brief     holds all classes and functionality exposed by the tfd library
 */
namespace tfd {
    namespace tests {
        class ObjectRadarTests; /**< object radar tests */
    }
    class ObjectRadarPrivate;   /**< internal data for object radar widget */

    /**
     * \class ObjectRadar
     * \brief implements a radar widget for use in remote controls to plot positions of various objects
     */
    class TFD_API ObjectRadar : public QWidget {
        Q_OBJECT
        Q_CLASSINFO("project", "tfd")
        Q_CLASSINFO("class", "ObjectRadar")
        Q_CLASSINFO("brief", "radar widget showing positions of objects relative to a fixed position")
        Q_CLASSINFO("author", "TophUwO <tophuwo01@gmail.com>")
        Q_CLASSINFO("version", "1.0.0")

    public:
        friend class tests::ObjectRadarTests;

        /**
         * \enum  Property
         * \brief enumeration for property fields
         * 
         * The values of this enumeration are used for the get*() and set*() family of methods.
         */
        enum class Property {
            /* view properties */
            AutoUpdate,      /**< [bool] automatically update view when property is changed */
            UpdateRate,      /**< [float] update rate per second (essentially FPS) {0.1, 240} */
            DefaultFont,     /**< [str] main display font */
            DefaultFontSize, /**< [int] font size for main display font */
            ForegroundColor, /**< [color] color used for lines and standard text */
            BackgroundColor, /**< [color] color used for backgrounds */
            RadarCenter,     /**< [point] {lat, lon} position of radar center */
            RadarAltitude,   /**< [float] altitude of radar center, in meters above sea-level */
            RadarRange,      /**< [size] radar range {min, max}, in meters relative to the radar center */

            /* object properties */
            Identifier,      /**< [str] object identifier */
            Type,            /**< [ObjectType] object type */
            Position,        /**< [point] position (latitude, longitude) */
            Color,           /**< [color] RGBA color */
            Area,            /**< [size] size of object (only for *area* type) */
            Altitude,        /**< [float] altitude of object (not for areas) */
            Visibility,      /**< [bool] object visible flag */

            __N__            /**< *only used internally* */
        };
        Q_ENUM(tfd::ObjectRadar::Property);
        /**
         * \enum  ObjectType
         * \brief enumeration for various object types representable on the object radar
         * 
         * The object radar supports adding a large number of object of various types to the
         * view in order to make navigation and flight planning easier. Objects are displayed
         * relative to the radar center's position.
         */
        enum class ObjectType {
            Vehicle, /**< aircraft object */
            Person,  /**< person object */
            Marker,  /**< marker object */
            Path,    /**< path object */
            Area,    /**< area object */

            __N__    /**< *only used internally* */
        };
        Q_ENUM(tfd::ObjectRadar::ObjectType);

        /**
         * \brief create a new object radar widget
         * \param [in] dim dimensions of the widget (width x height), in pixels
         * \param [in] parent pointer to the parent widget
         */
        explicit ObjectRadar(QSize const &dim, QWidget *parent = nullptr);
        ~ObjectRadar();

        /**
         * \brief  adds an object to the object radar
         * 
         * This function allows specifying the object's initial position and altitude. This can
         * be changed at any point using ObjectRadar::setProperty(QString, ObjectType, QVariant).
         * Each object will be represented by its own icon on the object radar. What icon is
         * shown exactly depends on the type of the object as well as its altitude relative to
         * the radar center.
         *
         * \param  [in] ident unique name for the object that will be shown on the object radar
         * \param  [in] type object type to create
         * \param  [in] pos (initial) [long, lat] position of object on object radar
         * \param  [in] alt (initial) altitude in meters above sea-level {def: 0}
         * \return *true* on success, *false* on error
         * \note   If an object with the same identifier as **ident** already exists, the function
         *         will fail.
         * \note   To disable altitude indicators for individual objects, set their altitude to *NaN*.
         */
        bool addObject(QString const &ident, ObjectRadar::ObjectType type, QPointF const &pos, float alt = 0.f);
        /**
         * \brief  removes an object from the object radar
         * \param  [in] ident identifier of the object to remove
         * \return *true* on success, *false* on failure
         * \note   This will instantly remove the object from the radar screen. To only make it invisible,
         *         use *ObjectRadar::setProperty("<...>", Property::Visibility, false)* in order to only
         *         make it invisible while keeping its state.
         */
        bool removeObject(QString const &ident);
        /**
         * \brief removes all objects from the object radar
         * \note  If no objects are present, this function does nothing.
         */
        void removeAllObjects();
        /**
         * \brief  checks whether an object with a given identifier exists
         * \param  [in] ident identifier of the object that is to be searched
         * \return *true* if the object could be found, *false* if not
         * \note   This function looks for exact matches for the object identifier.
         */
        bool hasObject(QString const &ident) const noexcept;
        /**
         * \brief  retrieves a copy of a view property with the given *property index*
         * \param  [in] prop property index to get value for
         * \return *QVariant* holding the property value
         * \note   This function is used for retrieving view properties. For retrieving
         *         object properties, use the overloaded version of this function with the
         *         *string identifier* as its first parameter.
         * \note   If the property could not be retrieved, using *QVariant::isValid* on the
         *         returned value will return *false*.
         * \see    ObjectRadar::Property
         * \see    ObjectRadar::getProperty(QString, ObjectRadar::Property)
         */
        QVariant getProperty(ObjectRadar::Property prop) const noexcept;
        /**
         * \brief  retrieves a copy of an object property with the given *property index*
         * \param  [in] prop property index to get value for
         * \return *QVariant* holding the property value
         * \note   This function is used for retrieving object properties. For retrieving
         *         view properties, use the overloaded version of this function with the
         *         *property index* as its first parameter.
         * \note   If the property could not be retrieved, using *QVariant::isValid* on the
         *         returned value will return *false*.
         * \see    ObjectRadar::Property
         * \see    ObjectRadar::getProperty(ObjectRadar::Property)
         */
        QVariant getProperty(QString const &ident, ObjectRadar::Property prop) const noexcept;
        /**
         * \brief  updates the value of a view property identified by the given *property index*
         * \param  [in] prop property index to update the value of
         * \param  [in] val new value for the property
         * \return *true* if the property was updated, *false* if the update failed
         * \note   This function is for updating view properties. To update a property of an object,
         *         use the overloaded version of this function with the *string identifier* as its
         *         first parameter.
         * \note   The function does type checking. If the type of the given new value does not
         *         match the value type for the property, the function fails.
         * \note   If the property index could not be found, the function fails.
         * \see    ObjectRadar::Property
         * \see    ObjectRadar::setProperty(QString, ObjectRadar::Property, QVariant)
         */
        bool setProperty(ObjectRadar::Property prop, QVariant const &val);
        /**
         * \brief  updates the value of an object property identified by the given *property index*
         * \param  [in] prop property index to update the value of
         * \param  [in] val new value for the property
         * \return *true* if the property was updated, *false* if the update failed
         * \note   The function does type checking. If the type of the given new value does not
         *         match the value type for the property, the function fails.
         * \note   If the property index could not be found, the function fails.
         * \see    ObjectRadar::Property
         * \see    ObjectRadar::setProperty(ObjectRadar::Property, QVariant)
         */
        bool setProperty(QString const &ident, ObjectRadar::Property prop, QVariant const &val);

    protected:
        /**
         * \brief reimplements the default widget paint event
         * 
         * This function has to be overridden as the widget is custom painted. Drawing never
         * happens outside this handler.
         * 
         * \param [in] pe additional info for the pending paint operation
         */
        virtual void paintEvent(QPaintEvent *pe) override;

    private:
        std::unique_ptr<ObjectRadarPrivate> m_data; /**< pointer to internal data */
    };


    /**
     * \brief  runs object radar unit tests
     * \return *0* if all tests were successful, or non-zero if at least one test failed
     * \note   This function will be called before the main window is shown, but only if
     *         the application is started with the '--run-tests' command-line option
     */
    TFD_EXTERN TFD_API int RunObjectRadarTests();
}


