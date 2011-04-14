

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2011, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


import QtQuick 1.0

Rectangle {
  id: dropMenu
  property real itemHeight: 32
  property real itemWidth: width
  property real floatHeight: childrenRect.height
  property real floatWidth: childrenRect.width
  property real rollDelay: 125
  property real initialYScale: 0


  function hide () {
    rollupScale.yScale = 0
  }
  function show () {
    rollupScale.yScale = 1
    console.log ("show menu at z = " + z )
  }
  transform: Scale {
    id: rollupScale
    xScale: 1
    yScale: initialYScale
    Behavior  on yScale {
      NumberAnimation { duration: rollDelay }
    }
  }

  width: itemWidth
  height: 200
  color: "green"
}
