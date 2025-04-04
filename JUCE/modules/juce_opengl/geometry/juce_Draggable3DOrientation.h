/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
/**
    Stores a 3D orientation, which can be rotated by dragging with the mouse.

    @tags{OpenGL}
*/
class Draggable3DOrientation
{
public:
    using VectorType      = Vector3D<float>;
    using QuaternionType  = Quaternion<float>;

    /** Creates a Draggable3DOrientation, initially set up to be aligned along the X axis. */
    Draggable3DOrientation (float objectRadius = 0.5f) noexcept
        : radius (jmax (0.1f, objectRadius)),
          quaternion (VectorType::xAxis(), 0)
    {
    }

    /** Creates a Draggable3DOrientation from a user-supplied quaternion. */
    Draggable3DOrientation (const Quaternion<float>& quaternionToUse,
                            float objectRadius = 0.5f) noexcept
        : radius (jmax (0.1f, objectRadius)),
          quaternion (quaternionToUse)
    {
    }

    /** Resets the orientation, specifying the axis to align it along. */
    void reset (const VectorType& axis) noexcept
    {
        quaternion = QuaternionType (axis, 0);
    }

    /** Sets the viewport area within which mouse-drag positions will occur.
        You'll need to set this rectangle before calling mouseDown. The centre of the
        rectangle is assumed to be the centre of the object that will be rotated, and
        the size of the rectangle will be used to scale the object radius - see setRadius().
    */
    void setViewport (Rectangle<int> newArea) noexcept
    {
        area = newArea;
    }

    /** Sets the size of the rotated object, as a proportion of the viewport's size.
        @see setViewport
    */
    void setRadius (float newRadius) noexcept
    {
        radius = jmax (0.1f, newRadius);
    }

    /** Begins a mouse-drag operation.
        You must call this before any calls to mouseDrag(). The position that is supplied
        will be treated as being relative to the centre of the rectangle passed to setViewport().
    */
    template <typename Type>
    void mouseDown (Point<Type> mousePos) noexcept
    {
        lastMouse = mousePosToProportion (mousePos.toFloat());
    }

    /** Continues a mouse-drag operation.
        After calling mouseDown() to begin a drag sequence, you can call this method
        to continue it.
    */
    template <typename Type>
    void mouseDrag (Point<Type> mousePos) noexcept
    {
        auto oldPos = projectOnSphere (lastMouse);
        lastMouse = mousePosToProportion (mousePos.toFloat());
        auto newPos = projectOnSphere (lastMouse);

        quaternion *= rotationFromMove (oldPos, newPos);
    }

    /** Returns the matrix that should be used to apply the current orientation.
        @see applyToOpenGLMatrix
    */
    Matrix3D<float> getRotationMatrix() const noexcept
    {
        return quaternion.getRotationMatrix();
    }

    /** Provides direct access to the quaternion. */
    QuaternionType& getQuaternion() noexcept
    {
        return quaternion;
    }

private:
    Rectangle<int> area;
    float radius;
    QuaternionType quaternion;
    Point<float> lastMouse;

    Point<float> mousePosToProportion (Point<float> mousePos) const noexcept
    {
        auto scale = jmin (area.getWidth(), area.getHeight()) / 2;

        // You must call setViewport() to give this object a valid window size before
        // calling any of the mouse input methods!
        jassert (scale > 0);

        return { (mousePos.x - (float) area.getCentreX()) / (float) scale,
                 ((float) area.getCentreY() - mousePos.y) / (float) scale };
    }

    VectorType projectOnSphere (Point<float> pos) const noexcept
    {
        auto radiusSquared = radius * radius;
        auto xySquared = pos.x * pos.x + pos.y * pos.y;

        return { pos.x, pos.y,
                 xySquared < radiusSquared * 0.5f ? std::sqrt (radiusSquared - xySquared)
                                                  : (radiusSquared / (2.0f * std::sqrt (xySquared))) };
    }

    QuaternionType rotationFromMove (const VectorType& from, const VectorType& to) const noexcept
    {
        auto rotationAxis = (to ^ from);

        if (rotationAxis.lengthIsBelowEpsilon())
            rotationAxis = VectorType::xAxis();

        auto d = jlimit (-1.0f, 1.0f, (from - to).length() / (2.0f * radius));

        return QuaternionType::fromAngle (2.0f * std::asin (d), rotationAxis);
    }
};

} // namespace juce
