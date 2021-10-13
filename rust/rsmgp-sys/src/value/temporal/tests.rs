// Copyright (c) 2016-2021 Memgraph Ltd. [https://memgraph.com]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use super::*;
use crate::memgraph::Memgraph;
use crate::mgp::mock_ffi::*;
use crate::testing::alloc::*;
use crate::{mock_mgp_once, with_dummy};
use libc::{c_void, free};
use serial_test::serial;

#[test]
#[serial]
fn test_from_naive_date() {
    let test_date = |date: NaiveDate| {
        mock_mgp_once!(
            mgp_date_from_parameters_context,
            move |date_params, _, date_ptr_ptr| unsafe {
                assert_eq!((*date_params).year, date.year());
                assert_eq!((*date_params).month as u32, date.month());
                assert_eq!((*date_params).day as u32, date.day());
                (*date_ptr_ptr) = alloc_mgp_date();
                mgp_error::MGP_ERROR_NO_ERROR
            }
        );
        mock_mgp_once!(mgp_date_destroy_context, |ptr| unsafe {
            free(ptr as *mut c_void);
        });

        with_dummy!(|memgraph: &Memgraph| {
            let _mgp_date = Date::from_naive_date(&date, &memgraph);
        });
    };
    test_date(NaiveDate::from_ymd(0, 1, 1));
    test_date(NaiveDate::from_ymd(1834, 1, 1));
    test_date(NaiveDate::from_ymd(1996, 12, 7));
    test_date(NaiveDate::from_ymd(9999, 12, 31));
}

#[test]
#[serial]
fn test_date_accessors() {
    let year = 1934;
    let month = 2;
    let day = 31;
    mock_mgp_once!(mgp_date_get_year_context, move |_, year_ptr| unsafe {
        (*year_ptr) = year;
        mgp_error::MGP_ERROR_NO_ERROR
    });
    mock_mgp_once!(mgp_date_get_month_context, move |_, month_ptr| unsafe {
        (*month_ptr) = month;
        mgp_error::MGP_ERROR_NO_ERROR
    });
    mock_mgp_once!(mgp_date_get_day_context, move |_, day_ptr| unsafe {
        (*day_ptr) = day;
        mgp_error::MGP_ERROR_NO_ERROR
    });

    with_dummy!(Date, |date: &Date| {
        assert_eq!(date.year(), year);
        assert_eq!(date.month() as i32, month);
        assert_eq!(date.day() as i32, day);
    });
}

#[test]
#[serial]
fn test_invalid_date() {
    let test_invalid_date = |date: NaiveDate| {
        with_dummy!(|memgraph: &Memgraph| {
            let result = Date::from_naive_date(&date, &memgraph);
            assert!(result.is_err());
            assert_eq!(
                result.err().unwrap(),
                Error::UnableToCreateDateFromNaiveDate
            );
        });
    };
    test_invalid_date(NaiveDate::from_ymd(-1, 12, 31));
    test_invalid_date(NaiveDate::from_ymd(10000, 1, 1));
}

#[test]
#[serial]
fn test_date_unable_to_allocate() {
    mock_mgp_once!(mgp_date_from_parameters_context, move |_, _, _| {
        mgp_error::MGP_ERROR_UNABLE_TO_ALLOCATE
    });

    with_dummy!(|memgraph: &Memgraph| {
        let error = Date::from_naive_date(&NaiveDate::from_num_days_from_ce(0), &memgraph);
        assert!(error.is_err());
        assert_eq!(error.err().unwrap(), Error::UnableToCreateDateFromNaiveDate);
    });
}

#[test]
#[serial]
fn test_from_naive_time() {
    let test_time = |time: NaiveTime, millis: i32, micros: i32| {
        mock_mgp_once!(
            mgp_local_time_from_parameters_context,
            move |local_time_params, _, local_time_ptr_ptr| unsafe {
                assert_eq!((*local_time_params).hour as u32, time.hour());
                assert_eq!((*local_time_params).minute as u32, time.minute());
                assert_eq!((*local_time_params).second as u32, time.second());
                assert_eq!((*local_time_params).millisecond, millis);
                assert_eq!((*local_time_params).microsecond, micros);
                (*local_time_ptr_ptr) = alloc_mgp_local_time();
                mgp_error::MGP_ERROR_NO_ERROR
            }
        );
        mock_mgp_once!(mgp_local_time_destroy_context, |ptr| unsafe {
            free(ptr as *mut c_void);
        });

        with_dummy!(|memgraph: &Memgraph| {
            let _mgp_local_time = LocalTime::from_naive_time(&time, &memgraph);
        });
    };
    test_time(NaiveTime::from_hms_micro(0, 0, 0, 0), 0, 0);
    test_time(NaiveTime::from_hms_micro(23, 59, 59, 999_999), 999, 999);
    test_time(NaiveTime::from_hms_micro(1, 2, 3, 444_555), 444, 555);
    // Leaps seconds handling
    test_time(NaiveTime::from_hms_micro(23, 59, 59, 1_999_999), 999, 999);
    test_time(NaiveTime::from_hms_micro(1, 2, 3, 1_444_555), 444, 555);
}

#[test]
#[serial]
fn test_local_time_accessors() {
    let hour = 23;
    let minute = 1;
    let second = 2;
    let millisecond = 3;
    let microsecond = 4;
    mock_mgp_once!(mgp_local_time_get_hour_context, move |_, hour_ptr| unsafe {
        (*hour_ptr) = hour;
        mgp_error::MGP_ERROR_NO_ERROR
    });
    mock_mgp_once!(
        mgp_local_time_get_minute_context,
        move |_, minute_ptr| unsafe {
            (*minute_ptr) = minute;
            mgp_error::MGP_ERROR_NO_ERROR
        }
    );
    mock_mgp_once!(
        mgp_local_time_get_second_context,
        move |_, second_ptr| unsafe {
            (*second_ptr) = second;
            mgp_error::MGP_ERROR_NO_ERROR
        }
    );
    mock_mgp_once!(
        mgp_local_time_get_millisecond_context,
        move |_, millisecond_ptr| unsafe {
            (*millisecond_ptr) = millisecond;
            mgp_error::MGP_ERROR_NO_ERROR
        }
    );
    mock_mgp_once!(
        mgp_local_time_get_microsecond_context,
        move |_, microsecond_ptr| unsafe {
            (*microsecond_ptr) = microsecond;
            mgp_error::MGP_ERROR_NO_ERROR
        }
    );

    with_dummy!(LocalTime, |time: &LocalTime| {
        assert_eq!(time.hour() as i32, hour);
        assert_eq!(time.minute() as i32, minute);
        assert_eq!(time.second() as i32, second);
        assert_eq!(time.millisecond() as i32, millisecond);
        assert_eq!(time.microsecond() as i32, microsecond);
    });
}

#[test]
#[serial]
fn test_local_time_unable_to_allocate() {
    mock_mgp_once!(mgp_local_time_from_parameters_context, move |_, _, _| {
        mgp_error::MGP_ERROR_UNABLE_TO_ALLOCATE
    });

    with_dummy!(|memgraph: &Memgraph| {
        let error =
            LocalTime::from_naive_time(&NaiveTime::from_num_seconds_from_midnight(0, 0), &memgraph);
        assert!(error.is_err());
        assert_eq!(
            error.err().unwrap(),
            Error::UnableToCreateLocalTimeFromNaiveTime
        );
    });
}
