/*
 * Copyright (c) 2012-2018 Frederic Julian
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:></http:>//www.gnu.org/licenses/>.
 */

package net.frju.flym.ui.views

import android.view.View

/**
 * Implement to handle an item being dragged.
 *
 * @author Eric Harlow
 */
interface DragNDropListener {
    /**
     * Called when a drag starts.
     *
     * @param itemView the view of the item to be dragged i.e. the drag view
     */
    fun onStartDrag(itemView: View)

    /**
     * Called when a drag is to be performed.
     *
     * @param x        horizontal coordinate of MotionEvent.
     * @param y        vertical coordinate of MotionEvent.
     */
    fun onDrag(x: Float, y: Float)

    /**
     * Called when a drag stops. Any changes in onStartDrag need to be undone here so that the view can be used in the list again.
     *
     * @param itemView the view of the item to be dragged i.e. the drag view
     */
    fun onStopDrag(itemView: View)

    /**
     * Called when an item is to be dropped.
     *
     * @param posFrom index item started at.
     * @param posTo   index to place item at.
     */
    fun onDrop(posFrom: Int, posTo: Int)
}
