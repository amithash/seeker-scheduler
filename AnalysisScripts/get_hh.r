 #*****************************************************************************\
 # FILE: get_hh.r
 # DESCRIPTION: DELETE
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Amithash Prasad                                              *
 #                                                                             *
 # This file is part of Seeker                                                 *
 #                                                                             *
 # Seeker is free software: you can redistribute it and/or modify it under the *
 # terms of the GNU General Public License as published by the Free Software   *
 # Foundation, either version 3 of the License, or (at your option) any later  *
 # version.                                                                    *
 #                                                                             *
 # This program is distributed in the hope that it will be useful, but WITHOUT *
 # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 # FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 # for more details.                                                           *
 #                                                                             *
 # You should have received a copy of the GNU General Public License along     *
 # with this program. If not, see <http://www.gnu.org/licenses/>.              *
 #*****************************************************************************/

hh['High','1400MHz'] = median(a$s1[a$workload == 'High'])
hh['High','1700MHz'] = median(a$s2[a$workload == 'High'])
hh['High','2000MHz'] = median(a$s3[a$workload == 'High'])
hh['High','2200MHz'] = median(a$s4[a$workload == 'High'])
hh['Low','1400MHz'] = median(a$s1[a$workload == 'Low'])
hh['Low','1700MHz'] = median(a$s2[a$workload == 'Low'])
hh['Low','2000MHz'] = median(a$s3[a$workload == 'Low'])
hh['Low','2200MHz'] = median(a$s4[a$workload == 'Low'])
hh['Low-High','2200MHz'] = median(a$s4[a$workload == 'Low-High'])
hh['Low-High','2000MHz'] = median(a$s3[a$workload == 'Low-High'])
hh['Low-High','1700MHz'] = median(a$s2[a$workload == 'Low-High'])
hh['Low-High','1400MHz'] = median(a$s1[a$workload == 'Low-High'])
hh['PHigh-PLow','1400MHz'] = median(a$s1[a$workload == 'PHigh-PLow'])
hh['PHigh-PLow','1700MHz'] = median(a$s2[a$workload == 'PHigh-PLow'])
hh['PHigh-PLow','2000MHz'] = median(a$s3[a$workload == 'PHigh-PLow'])
hh['PHigh-PLow','2200MHz'] = median(a$s4[a$workload == 'PHigh-PLow'])
hh['PLow-Low','2200MHz'] = median(a$s4[a$workload == 'PLow-Low'])
hh['PLow-Low','2000MHz'] = median(a$s3[a$workload == 'PLow-Low'])
hh['PLow-Low','1700MHz'] = median(a$s2[a$workload == 'PLow-Low'])
hh['PLow-Low','1400MHz'] = median(a$s1[a$workload == 'PLow-Low'])
hh['PLow-High','1400MHz'] = median(a$s1[a$workload == 'PLow-High'])
hh['PLow-High','1700MHz'] = median(a$s2[a$workload == 'PLow-High'])
hh['PLow-High','2000MHz'] = median(a$s3[a$workload == 'PLow-High'])
hh['PLow-High','2200MHz'] = median(a$s4[a$workload == 'PLow-High'])
