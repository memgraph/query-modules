use serial_test::serial;

use super::*;
use crate::mgp::mock_ffi::*;

#[test]
#[serial]
fn test_mgp_copy() {
    let ctx_1 = mgp_list_size_context();
    ctx_1.expect().times(1).returning(|_| 0);
    let ctx_2 = mgp_list_make_empty_context();
    ctx_2
        .expect()
        .times(1)
        .returning(|_, _| std::ptr::null_mut());

    let memgraph = Memgraph {
        ..Default::default()
    };
    unsafe {
        let value = List::mgp_copy(std::ptr::null_mut(), &memgraph);
        assert!(value.is_err());
    }
}

#[test]
#[serial]
fn test_make_empty() {
    let ctx_1 = mgp_list_make_empty_context();
    ctx_1
        .expect()
        .times(1)
        .returning(|_, _| std::ptr::null_mut());

    let memgraph = Memgraph {
        ..Default::default()
    };
    let value = List::make_empty(0, &memgraph);
    assert!(value.is_err());
}

// TODO(gitbuda): Figure out how + test list mgp_copy because it's quite complex.
// TODO(gitbuda): Figure out how + test list append and append_extend methods.

#[test]
#[serial]
fn test_size() {
    let ctx_1 = mgp_list_size_context();
    ctx_1.expect().times(1).returning(|_| 0);

    let memgraph = Memgraph {
        ..Default::default()
    };
    let list = List::new(std::ptr::null_mut(), &memgraph);
    let value = list.size();
    assert_eq!(value, 0);
}

#[test]
#[serial]
fn test_capacity() {
    let ctx_1 = mgp_list_capacity_context();
    ctx_1.expect().times(1).returning(|_| 0);

    let memgraph = Memgraph {
        ..Default::default()
    };
    let list = List::new(std::ptr::null_mut(), &memgraph);
    let value = list.capacity();
    assert_eq!(value, 0);
}

#[test]
#[serial]
fn test_value_at() {
    let ctx_1 = mgp_list_at_context();
    ctx_1
        .expect()
        .times(1)
        .returning(|_, _| std::ptr::null_mut());

    let memgraph = Memgraph {
        ..Default::default()
    };
    let list = List::new(std::ptr::null_mut(), &memgraph);
    let value = list.value_at(0);
    assert!(value.is_err());
}

#[test]
#[serial]
fn test_empty_list_iter() {
    let ctx_1 = mgp_list_size_context();
    ctx_1.expect().times(1).returning(|_| 0);

    let memgraph = Memgraph {
        ..Default::default()
    };
    let list = List::new(std::ptr::null_mut(), &memgraph);
    let iter = list.iter();
    assert!(iter.is_ok());

    let value = iter.unwrap().next();
    assert!(value.is_none());
}
