/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef CLIENTOBJCONTAINER_H_
#define CLIENTOBJCONTAINER_H_

class supla_client;
class supla_client_objcontainer {
 private:
  supla_client *client;
  void *arr;

 protected:
  static char arr_delcnd(void *ptr);
  void arr_clean(void);

 public:
  explicit supla_client_objcontainer(supla_client *client);
  virtual ~supla_client_objcontainer();

  int count(void);
};

#endif /* CLIENTOBJCONTAINER_H_ */
