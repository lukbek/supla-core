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

#include "action.h"
#include "string.h"

s_worker_action::s_worker_action(s_worker *worker) { this->worker = worker; }

s_worker_action::~s_worker_action() {}

bool s_worker_action::check_function_allowed(void) {
  int flist[FUNCTION_LIST_SIZE];
  memset((void *)flist, 0, FUNCTION_LIST_SIZE * sizeof(int));

  for (int a = 0; a < FUNCTION_LIST_SIZE; a++)
    if (flist[a] == worker->get_channel_func()) return true;

  worker->get_db()->set_result(worker->get_id(),
                               ACTION_EXECUTION_RESULT_CANCELLED);
  return false;
}

int s_worker_action::_retry_limit(void) { return retry_limit() * 2; }

int s_worker_action::_waiting_time_to_retry(void) {
  int time = waiting_time_to_retry();

  return time <= MIN_RETRY_TIME ? MIN_RETRY_TIME : time;
}

int s_worker_action::_waiting_time_to_check(void) {
  int time = waiting_time_to_check();

  if (time < MIN_CHECK_TIME) {
    time = MIN_CHECK_TIME;
  }

  if (time > _waiting_time_to_retry() - 1) {
    time = _waiting_time_to_retry() - 1;
  }

  return time;
}

void s_worker_action::_check_result(void) { bool success = check_result(); }

void s_worker_action::execute(void) {
  if (worker == NULL || worker->get_db() == NULL || !check_function_allowed())
    return;

  if (worker->get_retry_count() % 2 == 0) {  // SET
    do_action();
    worker->get_db()->set_retry(worker->get_id(), _waiting_time_to_check());
    return;
  }

  // CHECK
}
