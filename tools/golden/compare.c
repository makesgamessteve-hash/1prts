/* 1stPRTS golden image comparison. MIT License.
 *
 * usage: prts_golden_compare --ref DIR --out DIR [--diff DIR] [--expect-mismatch NAME]
 *
 * Every PNG in the reference folder must have a rendered PNG of the same name and size. A
 * pixel differs when any channel differs by more than 8 levels; an image fails when more
 * than 0.5% of its pixels differ (spec: Visual regression). Failing images get a difference
 * picture in the diff folder (the reference in grey, differing pixels in red).
 * With --expect-mismatch NAME only that image is compared, and the tool succeeds only if it
 * fails the comparison (this proves the check catches a real change). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx/image.h"
#include "prts/prts_fs.h"

#define TOLERANCE 8
#define MAX_FRACTION 0.005

typedef struct names {
    char list[256][128];
    int count;
} names;

static void collect(const char *name, void *user)
{
    names *n = user;
    size_t len = strlen(name);
    if (len > 4 && len < sizeof(n->list[0]) && strcmp(name + len - 4, ".png") == 0 && n->count < 256) {
        strcpy(n->list[n->count++], name);
    }
}

static int by_name(const void *a, const void *b) { return strcmp(a, b); }

static unsigned char *load(const char *dir, const char *name, int *w, int *h)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    char *data = NULL;
    size_t size = 0;
    if (!prts_fs_read_file(path, &data, &size)) return NULL;
    unsigned char *px = prts_image_decode(data, size, w, h);
    prts_free(data);
    return px;
}

/* Returns 1 if the image matches, 0 if it differs, -1 if it could not be compared. */
static int compare(const char *ref_dir, const char *out_dir, const char *diff_dir, const char *name)
{
    int rw, rh, ow, oh;
    unsigned char *ref = load(ref_dir, name, &rw, &rh);
    unsigned char *out = load(out_dir, name, &ow, &oh);
    int result = -1;
    if (!ref || !out) {
        printf("%-32s MISSING (%s)\n", name, ref ? "no rendered image" : "unreadable reference");
    } else if (rw != ow || rh != oh) {
        printf("%-32s SIZE %dx%d, reference %dx%d\n", name, ow, oh, rw, rh);
    } else {
        size_t n = (size_t)rw * (size_t)rh, bad = 0;
        int worst = 0;
        unsigned char *diff = malloc(n * 4);
        for (size_t i = 0; i < n; i++) {
            int d = 0;
            for (int c = 0; c < 3; c++) {
                int v = abs((int)ref[i * 4 + c] - (int)out[i * 4 + c]);
                if (v > d) d = v;
            }
            if (d > worst) worst = d;
            unsigned char grey = (unsigned char)((ref[i * 4] * 30 + ref[i * 4 + 1] * 59 + ref[i * 4 + 2] * 11) / 300);
            bool off = d > TOLERANCE;
            bad += off ? 1u : 0u;
            diff[i * 4 + 0] = off ? 255 : grey;
            diff[i * 4 + 1] = off ? 0 : grey;
            diff[i * 4 + 2] = off ? 0 : grey;
            diff[i * 4 + 3] = 255;
        }
        double fraction = (double)bad / (double)n;
        result = fraction <= MAX_FRACTION ? 1 : 0;
        printf("%-32s %s  %.3f%% of pixels differ (max %d levels)\n", name, result ? "ok  " : "FAIL", fraction * 100.0,
               worst);
        if (!result && diff_dir) {
            char path[1024];
            prts_fs_mkdirs(diff_dir);
            snprintf(path, sizeof(path), "%s/%s", diff_dir, name);
            prts_image_write_png(path, diff, rw, rh);
        }
        free(diff);
    }
    prts_image_free(ref);
    prts_image_free(out);
    return result;
}

int main(int argc, char **argv)
{
    const char *ref = NULL, *out = NULL, *diff = NULL, *expect = NULL;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--ref") == 0) ref = argv[i + 1];
        else if (strcmp(argv[i], "--out") == 0) out = argv[i + 1];
        else if (strcmp(argv[i], "--diff") == 0) diff = argv[i + 1];
        else if (strcmp(argv[i], "--expect-mismatch") == 0) expect = argv[i + 1];
    }
    if (!ref || !out) {
        fprintf(stderr, "usage: prts_golden_compare --ref DIR --out DIR [--diff DIR] [--expect-mismatch NAME]\n");
        return 2;
    }
    if (expect) {
        int r = compare(ref, out, NULL, expect);
        printf(r == 0 ? "the changed image was caught\n" : "expected '%s' to differ from its reference\n", expect);
        return r == 0 ? 0 : 1;
    }
    names refs;
    refs.count = 0;
    if (!prts_fs_enumerate(ref, collect, &refs) || refs.count == 0) {
        fprintf(stderr, "no reference images in %s (create them with tools/golden/update)\n", ref);
        return 1;
    }
    qsort(refs.list, (size_t)refs.count, sizeof(refs.list[0]), by_name);
    int failed = 0, matched = 0;
    for (int i = 0; i < refs.count; i++) {
        bool ok = compare(ref, out, diff, refs.list[i]) == 1;
        matched += ok ? 1 : 0;
        failed += ok ? 0 : 1;
    }
    names outs;
    outs.count = 0;
    prts_fs_enumerate(out, collect, &outs);
    for (int i = 0; i < outs.count; i++) {
        bool known = false;
        for (int k = 0; k < refs.count && !known; k++) known = strcmp(outs.list[i], refs.list[k]) == 0;
        if (!known) {
            printf("%-32s NEW (no reference; run tools/golden/update)\n", outs.list[i]);
            failed++;
        }
    }
    printf("%d of %d golden images match\n", matched, refs.count);
    return failed ? 1 : 0;
}
