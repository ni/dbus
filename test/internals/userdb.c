/*
 * Copyright © 2023 Collabora Ltd.
 * SPDX-License-Identifier: MIT
 */

#include <config.h>

#include <glib.h>

#include <dbus/dbus.h>
#include "dbus/dbus-sysdeps.h"
#include "test-utils-glib.h"

#ifdef DBUS_UNIX
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "dbus/dbus-sysdeps-unix.h"
#include "dbus/dbus-userdb.h"
#endif

typedef struct
{
  int dummy;
} Fixture;

static void
setup (Fixture *f G_GNUC_UNUSED,
       gconstpointer context G_GNUC_UNUSED)
{
}

static void
test_info_from_uid (Fixture *f,
                    gconstpointer context G_GNUC_UNUSED)
{
#if defined(DBUS_UNIX) && defined(DBUS_ENABLE_EMBEDDED_TESTS)
  DBusString homedir = _DBUS_STRING_INIT_INVALID;
#endif
  DBusError error = DBUS_ERROR_INIT;
  dbus_gid_t *gids = NULL;
  int n_gids = -1;
  dbus_bool_t ret;
#ifdef DBUS_UNIX
  int i;
  /* arbitrarily chosen, probably isn't a valid uid */
  const dbus_uid_t not_a_uid = 31337;
#endif

  /* We assume that uid 0 (root) is available on all Unix systems,
   * so this should succeed */
  ret = _dbus_unix_groups_from_uid (0, &gids, &n_gids, &error);

#ifdef DBUS_UNIX
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_assert_cmpint (n_gids, >=, 0);

  g_test_message ("Groups of uid 0:");

  for (i = 0; i < n_gids; i++)
    {
      g_test_message ("[%d]: %ld", i, (long) gids[i]);
      g_assert_cmpint (gids[i], >=, 0);
    }

#ifdef DBUS_ENABLE_EMBEDDED_TESTS
  if (!_dbus_string_init (&homedir))
    test_oom ();

  ret = _dbus_homedir_from_uid (0, &homedir, &error);
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_test_message ("Home directory of uid 0: %s", _dbus_string_get_const_data (&homedir));
#endif
#else
  g_assert_cmpstr (error.name, ==, DBUS_ERROR_NOT_SUPPORTED);
  g_assert_false (ret);
  g_test_message ("Getting Unix groups on Windows failed as expected: %s: %s",
                  error.name, error.message);
  g_assert_null (gids);
  g_assert_cmpint (n_gids, <=, 0);
#endif

  dbus_free (gids);
  dbus_error_free (&error);

#ifdef DBUS_UNIX
  /* Assume that the current uid is something sensible */
  ret = _dbus_unix_groups_from_uid (geteuid (), &gids, &n_gids, &error);
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_assert_cmpint (n_gids, >=, 0);

  g_test_message ("Groups of uid %ld:", (long) geteuid ());

  for (i = 0; i < n_gids; i++)
    {
      g_test_message ("[%d]: %ld", i, (long) gids[i]);
      g_assert_cmpint (gids[i], >=, 0);
    }

  g_test_message ("Total: %i groups", n_gids);

  dbus_free (gids);
  dbus_error_free (&error);

#ifdef DBUS_ENABLE_EMBEDDED_TESTS
  if (!_dbus_string_set_length (&homedir, 0))
    test_oom ();

  ret = _dbus_homedir_from_uid (geteuid (), &homedir, &error);
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_test_message ("Home directory of current uid: %s", _dbus_string_get_const_data (&homedir));
#endif

  errno = 0;

  if (getpwuid (not_a_uid) == NULL)
    {
      g_test_message ("uid " DBUS_UID_FORMAT " doesn't exist: %s",
                      not_a_uid, errno == 0 ? "(no errno)" : g_strerror (errno));
      ret = _dbus_unix_groups_from_uid (not_a_uid, &gids, &n_gids, &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_test_message ("Getting groups from non-uid failed as expected: %s: %s",
                      error.name, error.message);
      /* The Unix implementation always clears gids/n_gids,
       * even on failure, and even if they were uninitialized */
      g_assert_null (gids);
      g_assert_cmpint (n_gids, ==, 0);

      dbus_free (gids);
      dbus_error_free (&error);

#ifdef DBUS_ENABLE_EMBEDDED_TESTS
      if (!_dbus_string_set_length (&homedir, 0))
        test_oom ();

      ret = _dbus_homedir_from_uid (not_a_uid, &homedir, &error);
      g_assert_nonnull (error.message);
      g_assert_nonnull (error.name);
      g_assert_false (ret);
      g_test_message ("Getting home directory from non-uid failed as expected: %s: %s",
                      error.name, error.message);
#endif
    }
  else
    {
      g_test_skip_printf ("against our expectations, uid " DBUS_UID_FORMAT
                          " exists on this system",
                          not_a_uid);
    }
#endif

#if defined(DBUS_UNIX) && defined(DBUS_ENABLE_EMBEDDED_TESTS)
  _dbus_string_free (&homedir);
#endif
}

static void
test_group_id_from_name (Fixture *f,
                         gconstpointer context G_GNUC_UNUSED)
{
  DBusError error = DBUS_ERROR_INIT;
  dbus_bool_t ret;
  dbus_gid_t gid = -1;
  DBusString name;

  /* This is a group that exists on most Unix systems */
  _dbus_string_init_const (&name, "bin");

  ret = _dbus_parse_unix_group_from_config (&name, &gid, &error);

#ifdef DBUS_UNIX
  if (getgrnam (_dbus_string_get_const_data (&name)) != NULL)
    {
      test_assert_no_error (&error);
      g_assert_true (ret);
      g_assert_cmpint (gid, >=, 0);
    }
#else
  g_assert_cmpstr (error.name, ==, DBUS_ERROR_NOT_SUPPORTED);
  g_assert_false (ret);
  g_test_message ("Parsing Unix group on Windows failed as expected: %s: %s",
                  error.name, error.message);
  g_assert_cmpint (gid, <, 0);
#endif

#ifdef DBUS_UNIX
  /* This function only exists on Unix */
  ret = _dbus_get_group_id (&name, &gid, &error);

  if (getgrnam (_dbus_string_get_const_data (&name)) != NULL)
    {
      test_assert_no_error (&error);
      g_assert_true (ret);
      g_assert_cmpint (gid, >=, 0);
    }

  /* arbitrarily chosen, probably isn't a valid group name */
  _dbus_string_init_const (&name, "not-a-group");

  if (getpwnam (_dbus_string_get_const_data (&name)) == NULL)
    {
      gid = -1;
      ret = _dbus_parse_unix_group_from_config (&name, &gid, &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_assert_cmpint (gid, <, 0);
      g_test_message ("Parsing nonexistent group failed as expected: %s: %s",
                      error.name, error.message);
      dbus_error_free (&error);

      gid = -1;
      ret = _dbus_get_group_id (&name, &gid, &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_test_message ("Getting gid of nonexistent group failed as expected: %s: %s",
                      error.name, error.message);
      g_assert_cmpint (gid, <, 0);
      dbus_error_free (&error);
    }
  else
    {
      g_test_skip ("our improbably-named group exists?!");
    }
#endif
}

static void
test_user_id_from_name (Fixture *f,
                        gconstpointer context G_GNUC_UNUSED)
{
  DBusError error = DBUS_ERROR_INIT;
  dbus_bool_t ret;
  dbus_uid_t uid = -1;
  dbus_gid_t gid = -1;
  DBusString username;

  /* We assume here that root is always uid 0 on Unix */
  _dbus_string_init_const (&username, "root");

  /* We assume that uid 0 (root) is available on all Unix systems,
   * so this should succeed */
  ret = _dbus_verify_daemon_user (_dbus_string_get_const_data (&username), &error);

#ifdef DBUS_UNIX
  test_assert_no_error (&error);
  g_assert_true (ret);
#else
  /* TODO: Surely this should fail for any username on Windows? At the moment,
   * it succeeds for any username... for now we make no assertion either way */
  dbus_error_free (&error);
#endif

  ret = _dbus_parse_unix_user_from_config (&username, &uid, &error);

#ifdef DBUS_UNIX
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_assert_cmpint (uid, ==, 0);
#else
  g_assert_cmpstr (error.name, ==, DBUS_ERROR_NOT_SUPPORTED);
  g_assert_false (ret);
  g_test_message ("Parsing Unix user on Windows failed as expected: %s: %s",
                  error.name, error.message);
  g_assert_cmpint (uid, <, 0);
#endif

#ifdef DBUS_UNIX
  uid = -1;
  gid = -1;
  ret = _dbus_get_user_id_and_primary_group (&username, &uid, &gid, &error);
  test_assert_no_error (&error);
  g_assert_true (ret);
  g_assert_cmpint (uid, ==, 0);
  g_assert_cmpint (gid, >=, 0);

  /* arbitrarily chosen, probably isn't a valid username */
  _dbus_string_init_const (&username, "not-a-user");

  if (getpwnam (_dbus_string_get_const_data (&username)) == NULL)
    {
      ret = _dbus_verify_daemon_user (_dbus_string_get_const_data (&username), &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_test_message ("Verifying nonexistent user failed as expected: %s: %s",
                      error.name, error.message);
      dbus_error_free (&error);

      uid = -1;
      ret = _dbus_parse_unix_user_from_config (&username, &uid, &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_assert_cmpint (uid, <, 0);
      g_test_message ("Parsing nonexistent user failed as expected: %s: %s",
                      error.name, error.message);
      dbus_error_free (&error);

      uid = -1;
      gid = -1;
      ret = _dbus_get_user_id_and_primary_group (&username, &uid, &gid, &error);
      g_assert_nonnull (error.name);
      g_assert_nonnull (error.message);
      g_assert_false (ret);
      g_test_message ("Getting uid/gid of nonexistent user failed as expected: %s: %s",
                      error.name, error.message);
      g_assert_cmpint (uid, <, 0);
      g_assert_cmpint (gid, <, 0);
      dbus_error_free (&error);
    }
  else
    {
      g_test_skip ("our improbably-named user exists?!");
    }
#endif
}

static void
teardown (Fixture *f G_GNUC_UNUSED,
          gconstpointer context G_GNUC_UNUSED)
{
}

int
main (int argc,
      char **argv)
{
  int ret;

  test_init (&argc, &argv);

  g_test_add ("/userdb/info_from_uid",
              Fixture, NULL, setup, test_info_from_uid, teardown);
  g_test_add ("/userdb/group_id_from_name",
              Fixture, NULL, setup, test_group_id_from_name, teardown);
  g_test_add ("/userdb/user_id_from_name",
              Fixture, NULL, setup, test_user_id_from_name, teardown);

  ret = g_test_run ();
  dbus_shutdown ();
  return ret;
}
