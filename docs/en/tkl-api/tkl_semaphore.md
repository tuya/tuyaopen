# Semaphore

The file `tkl_semaphore.c` is used for creating and managing semaphores to implement task synchronization or event notification between tasks in embedded systems or multi-tasking operating systems. This file provides interfaces for creating semaphores, waiting on semaphores, posting to semaphores, and releasing semaphores. The file is also auto-generated by the TuyaOS and reserves areas for developers to implement their code.

## API Description

### tkl_semaphore_create_init

```c
OPERATE_RET tkl_semaphore_create_init(TKL_SEM_HANDLE *handle, uint32_t sem_cnt, uint32_t sem_max);
```

#### Function

Create and initialize a counting semaphore.

#### Parameters

- `handle`: Output parameter to receive the created semaphore handle.
- `sem_cnt`: The initial count of the semaphore.
- `sem_max`: The maximum count of the semaphore.

#### Return Value

A return value of `OPRT_OK` indicates that the semaphore was successfully created, other return values indicate an error. See `tuya_error_code.h` for specific error codes.

### tkl_semaphore_wait

```c
OPERATE_RET tkl_semaphore_wait(const TKL_SEM_HANDLE handle, uint32_t timeout);
```

#### Function

Wait for a semaphore.

#### Parameters

- `handle`: Semaphore handle.
- `timeout`: Timeout duration for waiting, in milliseconds. `TKL_SEM_WAIT_FOREVER` indicates to wait indefinitely until the semaphore is obtained.

#### Return Value

`OPRT_OK` indicates that the semaphore was successfully obtained, `OPRT_OS_ADAPTER_SEM_WAIT_TIMEOUT` indicates a timeout occurred, other return values indicate an error. See `tuya_error_code.h` for specific error codes.

### tkl_semaphore_post

```c
OPERATE_RET tkl_semaphore_post(const TKL_SEM_HANDLE handle);
```

#### Function

Post (release) a semaphore, incrementing the semaphore's count.

#### Parameters

- `handle`: Semaphore handle.

#### Return Value

`OPRT_OK` indicates that the semaphore was successfully posted, other return values indicate an error. Detailed error codes can be queried in `tuya_error_code.h`.

### tkl_semaphore_release

```c
OPERATE_RET tkl_semaphore_release(const TKL_SEM_HANDLE handle);
```

#### Function

Release and delete a semaphore.

#### Parameters

- `handle`: Semaphore handle.

#### Return Value

`OPRT_OK` indicates that the resources were successfully released, other return values indicate an error. Detailed information can be obtained from `tuya_error_code.h`.
```