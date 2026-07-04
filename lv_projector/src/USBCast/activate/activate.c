/*

Copyright (c) 2008-2019 Allwinner Technology Co. Ltd.. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include "aw_mirror_interface.h"

#define ACTIVATE_STATE_FAIL         0
#define ACTIVATE_STATE_OK           1

struct ActivateServiceS *activate_dev_t = NULL;
void *activate_libFd = NULL;
static int8_t activate_state;

int get_activate_state(void)
{
    return activate_state;
}

int set_activate_state(int8_t state)
{
    activate_state = state;
    return 0;
}

static int activate_register(void)
{
    if (activate_dev_t)
        return 0;

    activate_libFd = dlopen(WIRED_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!activate_libFd) {
        printf("dlopen activate lib %s, error: %s\n", WIRED_LIB, dlerror());
        return -1;
    }

    ActivateServiceGetInstance activate = (ActivateServiceGetInstance)dlsym(activate_libFd, "ActivateServiceGetInstance");
    if (activate) {
        activate_dev_t = activate();
        return 0;
    }

    return -1;
}

static int activate_unregister(void)
{
    if (activate_libFd)
        dlclose(activate_libFd);

    activate_libFd = NULL;
    return 0;
}

static int __Activate_Event_CB(ActivateEventE enEvent)
{
    switch (enEvent) {
        case ACTIVATE_OK:
            set_activate_state(ACTIVATE_STATE_OK);
            del_activate_icon();
            break;
    }
    return 0;
}

int AWActivate_init()
{
    set_activate_state(ACTIVATE_STATE_FAIL);

    activate_register();
    if (activate_dev_t && activate_dev_t->init)
        activate_dev_t->init((void *)__Activate_Event_CB);

    return 0;
}



