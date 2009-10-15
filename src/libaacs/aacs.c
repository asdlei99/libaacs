
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <openssl/aes.h>

#include "aacs.h"
#include "crypto.h"
#include "mmc.h"
#include "../util/macro.h"
#include "../file/file.h"

int _calc_pk(AACS_KEYS *aacs);
int _calc_mk(AACS_KEYS *aacs, const char *path);
int _calc_vuk(AACS_KEYS *aacs, const char *path);
int _calc_uks(AACS_KEYS *aacs, const char *path);
int _validate_pk(uint8_t *pk, uint8_t *cvalue, uint8_t *uv, uint8_t *vd, uint8_t *mk);
int _verify_ts(uint8_t *buf, size_t size);

int _calc_mk(AACS_KEYS *aacs, const char *path)
{
    int a, num_uvs = 0;
    char f_name[100];
    size_t len;
    uint8_t *buf = NULL, *rec, *uvs, *key_pos;
    uint16_t num_pks;
    MKB *mkb = NULL;

    snprintf(f_name, 100, "%s/AACS/MKB_RO.inf", path);

    mkb = mkb_open(f_name);

    uvs = mkb_subdiff_records(mkb, &len);
    rec = uvs;
    while (rec < buf + len) {
        if (rec[0] & 0xc0)
            break;
        rec += 5;
        num_uvs++;
    }

    rec = mkb_cvalues(mkb, &len);
    key_pos = keyfile_record(aacs->kf, KF_PK_ARRAY, &num_pks, NULL);
    while (key_pos < aacs->pks + num_pks * 16) {
        memcpy(aacs->pk, key_pos, 16);

        for (a = 0; a < num_uvs; a++)
            if (_validate_pk(aacs->pk, rec + a * 16, uvs + 1 + a * 5, mkb_mk_dv(mkb), aacs->mk)) {
                mkb_close(mkb);
                X_FREE(buf);
                return 1;
            }

        key_pos += 16;
    }

    mkb_close(mkb);
    X_FREE(buf);

    return 0;
}

int _calc_vuk(AACS_KEYS *aacs, const char *path)
{
    int a;
    AES_KEY aes;
    uint8_t vid[16];
    MMC* mmc = NULL;

    if ((mmc = mmc_open(path,
            keyfile_record(aacs->kf, KF_HOST_PRIV_KEY, NULL, NULL),
            keyfile_record(aacs->kf, KF_HOST_CERT, NULL, NULL),
            keyfile_record(aacs->kf, KF_HOST_NONCE, NULL, NULL),
            keyfile_record(aacs->kf, KF_HOST_KEY_POINT, NULL, NULL)))) {
        if (mmc_read_vid(mmc)) {
            AES_set_decrypt_key(aacs->mk, 128, &aes);
            AES_decrypt(vid, aacs->vuk, &aes);

            for (a = 0; a < 16; a++) {
                aacs->vuk[a] ^= vid[a];
            }

            mmc_close(mmc);

            return 1;
        }

        mmc_close(mmc);
    }

    return 0;
}

int _calc_uks(AACS_KEYS *aacs, const char *path)
{
    AES_KEY aes;
    FILE_H *fp = NULL;
    unsigned char buf[16];
    char f_name[100];
    off_t f_pos;

    snprintf(f_name, 100, "/%s/AACS/Unit_Key_RO.inf", path);

    if ((fp = file_open(f_name, "rb"))) {
        file_read(fp, buf, 4);

        f_pos = MKINT_BE32(buf) + 48;

        file_seek(fp, f_pos, SEEK_SET);
        file_read(fp, buf, 16);

        AES_set_decrypt_key(aacs->vuk, 128, &aes);
        AES_decrypt(buf, aacs->uks, &aes);

        file_close(fp);

        return 1;
    }

    return 0;
}

int _validate_pk(uint8_t *pk, uint8_t *cvalue, uint8_t *uv, uint8_t *vd, uint8_t *mk)
{
    int a;
    AES_KEY aes;
    uint8_t dec_vd[16];

    AES_set_decrypt_key(pk, 128, &aes);
    AES_decrypt(cvalue, mk, &aes);

    for (a = 0; a < 4; a++) {
        mk[a + 12] ^= uv[a];
    }

    AES_set_decrypt_key(mk, 128, &aes);
    AES_decrypt(vd, dec_vd, &aes);

    if (!memcmp(dec_vd, "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8)) {
        return 1;
    }

    return 0;
}

AACS_KEYS *aacs_open(const char *path, const char *keyfile_path)
{
    AACS_KEYS *aacs = malloc(sizeof(AACS_KEYS));

    aacs->kf = NULL;
    if ((aacs->kf = keyfile_open(keyfile_path))) {
        _calc_pk(aacs);
        _calc_mk(aacs, path);
        _calc_vuk(aacs, path);
        _calc_uks(aacs, path);

        return aacs;
    }

    return NULL;
}

void aacs_close(AACS_KEYS *aacs)
{
    keyfile_close(aacs->kf);

    X_FREE(aacs);
}

int aacs_decrypt_unit(AACS_KEYS *aacs, uint8_t *buf)
{
    int a;
    AES_KEY aes;
    uint8_t key[16], iv[] = { 0x0b, 0xa0, 0xf8, 0xdd, 0xfe, 0xa6, 0x1f, 0xb3, 0xd8, 0xdf, 0x9f, 0x56, 0x6a, 0x05, 0x0f, 0x78 };

    AES_set_encrypt_key(aacs->uks, 128, &aes);
    AES_encrypt(buf, key, &aes);

    for (a = 0; a < 16; a++) {
        key[a] ^= buf[a];
    }

    AES_set_decrypt_key(key, 128, &aes);
    AES_cbc_encrypt(buf + 16, buf + 16, 6144 - 16, &aes, iv, 0);

    return 1;
}
