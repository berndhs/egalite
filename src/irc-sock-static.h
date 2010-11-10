#ifndef IRC_SOCK_STATIC
#define IRC_SOCK_STATIC

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
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
namepsace egalite
{

class IrcSock;

class IrcSockStatic {

private:


  static void TransformPRIVMSG (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);
  static void TransformJOIN    (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);
  static void TransformDefault (IrcSock * context,
                                QString & result, 
                                QString & first, 
                                QString & rest);

  static void ReceiveNumeric (IrcSock * context,
                         const QString & first,
                         const QString & num,
                         const QString & rest);
  static void ReceivePING (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceivePRIVMSG (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveJOIN (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceivePART (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveTOPIC (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void Receive332 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void Receive353 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void Receive366 (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveIgnore (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);
  static void ReceiveDefault (IrcSock * context,
                         const QString & first,
                         const QString & cmd,
                         const QString & rest);

} // namespace

