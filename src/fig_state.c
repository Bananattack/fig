#include <stdlib.h>
#include <fig.h>

struct fig_state {
    const char *error;
    fig_allocator_t alloc;
    void *ud;
};

static void *default_alloc(void *ud, void *ptr, size_t old_size, size_t new_size) {
    (void) ud;

    if((ptr != NULL && old_size == 0)
    || (ptr == NULL && old_size != 0)) {
        return NULL;
    }

    if(new_size == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, new_size);
    }
}

fig_state *fig_create_state(void) {
    return fig_create_custom_state(default_alloc, NULL);
}

fig_state *fig_create_custom_state(fig_allocator_t alloc, void *ud) {
    fig_state *self = (fig_state *) alloc(ud, NULL, 0, sizeof(fig_state));
    if(self != NULL) {
        self->error = NULL;
        self->alloc = alloc;
        self->ud = ud;
    }
    return self;
}

const char *fig_state_get_error(fig_state *self) {
    return self->error;
}

void fig_state_set_error(fig_state *self, const char* message) {
    self->error = message;
}

void fig_state_set_error_allocation_failed(fig_state *self) {
    fig_state_set_error(self, "allocation failed");
}

fig_allocator_t fig_state_get_allocator(fig_state *self) {
    return self->alloc;
}

void *fig_state_get_userdata(fig_state *self) {
    return self->ud;
}

void fig_state_free(fig_state *self) {
    if(self != NULL) {
        self->alloc(self->ud, self, sizeof(fig_state), 0);
    }
}