#include "Forte.h"
#include "ansi_test.h"

// main
int main(int argc, char *argv[])
{
    FileSystem *fs = FileSystem::get();
    FString stmp, cwd, s1, s2;
    struct stat st;
    int err = 0;

    // logging
    CLogManager log_mgr;
    log_mgr.BeginLogging("filesystem.log");
    log_mgr.SetGlobalLogMask(HLOG_NODEBUG & ~(HLOG_INFO));

    // init trace
    FTrace::enable();

    // test path functions
    TEST(fs->make_rel_path("/foo/bar/baz", "/foo/fred/barney"),
         "../../fred/barney",
         "make_rel_path(1)");

    TEST(fs->make_rel_path("/foo/bar/baz", "/fred/barney"),
         "../../../fred/barney",
         "make_rel_path(2)");

    TEST(fs->make_rel_path("/foo/bar/baz", "/foo/bar/baz/fred"),
         "fred",
         "make_rel_path(3)");

    TEST(fs->make_rel_path("/foo/bar/baz", "/foo/bar/baz/fred/barney"),
         "fred/barney",
         "make_rel_path(4)");

    TEST(fs->make_rel_path("/foo/bar/baz", "/foo/bar/baz/./fred/barney"),
         "./fred/barney",
         "make_rel_path(5)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "../../fred/barney"),
         "/foo/fred/barney",
         "resolve_rel_path(1)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "../../../fred/barney"),
         "/fred/barney",
         "resolve_rel_path(2)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "fred"),
         "/foo/bar/baz/fred",
         "resolve_rel_path(3)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "fred/barney"),
         "/foo/bar/baz/fred/barney",
         "resolve_rel_path(4)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "./fred/barney"),
         "/foo/bar/baz/./fred/barney",
         "resolve_rel_path(5)");

    TEST(fs->resolve_rel_path("/foo/bar/baz", "/fred/barney"),
         "/fred/barney",
         "resolve_rel_path(6)");

    // test getcwd
    TEST_NO_EXP(cwd = fs->getcwd(), "getcwd(1)");
    TEST(cwd, getenv("PWD"), "getcwd(2)");

    // test symlink functions
    system("touch ._baz");
    TEST_NO_EXP(fs->symlink("._baz", "._bar"), "symlink(1)");
    TEST_NO_EXP(fs->symlink("._bar", "._foo"), "symlink(2)");
    TEST_NO_EXP(fs->symlink("/nowhere", "._fred"), "symlink(3)");

    TEST(fs->resolve_symlink("._bar"), cwd + "/._baz", "resolve_symlink(1)");
    TEST(fs->resolve_symlink("._foo"), cwd + "/._baz", "resolve_symlink(2)");
    TEST_NO_EXP(fs->unlink("._baz"), "unlink(1)");
    TEST_EXP(fs->resolve_symlink("._foo"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||" + cwd + "/._baz",
             "resolve_symlink(3)");
    TEST_NO_EXP(fs->unlink("._bar"), "unlink(2)");
    TEST_EXP(fs->resolve_symlink("._foo"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||" + cwd  + "/._bar",
             "resolve_symlink(4)");
    TEST_NO_EXP(fs->unlink("._foo"), "unlink(3)");
    TEST_EXP(fs->resolve_symlink("._foo"), "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||._foo",
             "resolve_symlink(5)");
    TEST_NO_EXP(fs->unlink("._fred"), "unlink(4)");
    TEST_NO_EXP(fs->unlink("._fred"), "unlink(5)");

    symlink("._loop0", "._loop1");
    symlink("._loop1", "._loop0");
    TEST_EXP(fs->resolve_symlink("._loop0"),
             "FORTE_RESOLVE_SYMLINK_LOOP|||._loop0|||" + cwd + "/._loop1",
             "resolve_symlink(6)");
    TEST_EXP(fs->resolve_symlink("._loop1"),
             "FORTE_RESOLVE_SYMLINK_LOOP|||._loop1|||" + cwd + "/._loop0",
             "resolve_symlink(7)");
    TEST_NO_EXP(fs->unlink("._loop0"), "unlink(6)");
    TEST_NO_EXP(fs->unlink("._loop1"), "unlink(7)");

    // test link to broken symlink
    TEST_NO_EXP(fs->symlink("broken", "broken.symlink"), "symlink(4)");
    TEST(fs->readlink("broken.symlink"), "broken", "readlink(1)");
    TEST_EXP(fs->readlink("not.a.symlink"),
             "FORTE_READ_SYMLINK_FAIL|||not.a.symlink|||No such file or directory",
             "readlink(2)");
    TEST_NO_EXP(fs->link("broken.symlink", "broken.symlink.hardlink"), "link(1)");
    TEST_NO_EXP(fs->unlink("broken.symlink"), "unlink(8)");
    TEST_NO_EXP(fs->unlink("broken.symlink.hardlink"), "unlink(9)");

    // test double-resolve
    TEST_NO_EXP(fs->mkdir("._dir0"), "mkdir(1)");
    TEST_NO_EXP(fs->symlink("._dir0/back", "._forward"), "symlink(5)");
    TEST_NO_EXP(fs->symlink("../._foo", "._dir0/back"), "symlink(6)");
    system("touch ._foo");
    TEST(fs->resolve_symlink("._forward"), cwd + "/._foo", "resolve_symlink(8)");
    TEST_NO_EXP(fs->unlink("._foo"), "unlink(10)");
    TEST_NO_EXP(fs->unlink("._forward"), "unlink(11)");
    TEST_EXP(fs->unlink("._dir0", false), "FORTE_UNLINK_FAIL|||._dir0|||Directory not empty",
             "unlink(13)");
    TEST_NO_EXP(fs->unlink("._dir0", true), "unlink(12)");
    TEST_NO_EXP(err = fs->stat("._dir0", &st), "stat(1)");
    TEST(err, -1, "unlink(14)");
    TEST_NO_EXP(err = fs->stat(".", &st), "stat(2)");
    TEST(err, 0, "stat(3)");
    TEST(S_ISDIR(st.st_mode), true, "stat(4)");

    // test full resolve
    TEST_NO_EXP(fs->mkdir("._dir0"), "mkdir(2)");
    TEST_NO_EXP(fs->mkdir("._dir1"), "mkdir(3)");
    TEST_NO_EXP(fs->mkdir("._dir1"), "mkdir(4)");
    TEST_NO_EXP(fs->touch("._dir0/foo"), "touch(1)");
    TEST_NO_EXP(fs->symlink("._dir0", "._s0"), "symlink(7)");
    TEST_NO_EXP(fs->symlink("._dir1", "._s1"), "symlink(8)");
    TEST_NO_EXP(fs->symlink("._s0/foo", "._s2"), "symlink(9)");
    TEST_NO_EXP(fs->symlink("../._s2", "._dir1/._s3"), "symlink(10)");
    TEST_NO_EXP(fs->symlink("._s1/._s3", "._s4"), "symlink(11)");
    TEST_NO_EXP(stmp = fs->fully_resolve_symlink("._s2"), "fully_resolve_symlink(1)");
    TEST(stmp, cwd + "/._dir0/foo", "fully_resolve_symlink(2)");
    TEST_NO_EXP(stmp = fs->fully_resolve_symlink("._dir1/._s3"), "fully_resolve_symlink(3)");
    TEST(stmp, cwd + "/._dir0/foo", "fully_resolve_symlink(4)");
    TEST_NO_EXP(stmp = fs->fully_resolve_symlink("._s4"), "fully_resolve_symlink(5)");
    TEST(stmp, cwd + "/._dir0/foo", "fully_resolve_symlink(6)");
    system("rm -f ._dir0/foo");
    TEST_EXP(fs->fully_resolve_symlink("._s4"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._s4|||" + cwd + "/._dir0/foo",
             "fully_resolve_symlink(7)");
    system("rm -rf ._dir0 ._dir1 ._s0 ._s1 ._s2 ._s4");

    // test touch
    TEST_NO_EXP(fs->touch(cwd + "/._foo"), "touch(2)");
    TEST_NO_EXP(fs->touch(cwd + "/._foo"), "touch(3)");
    TEST_NO_EXP(fs->touch("._foo"), "touch(4)");
    system("rm -f ._foo");

    // test deep_copy
    TEST_NO_EXP(fs->mkdir(cwd + "/._from/1/2/3", 0777, true), "mkdir(5)");
    TEST_NO_EXP(fs->mkdir(cwd + "/._from/4/5/6", 0777, true), "mkdir(6)");
    TEST_NO_EXP(fs->touch(cwd + "/._from/empty"), "touch(5)");
    TEST_NO_EXP(fs->touch(cwd + "/._from/1/empty"), "touch(6)");
    TEST_NO_EXP(fs->touch(cwd + "/._from/4/5/empty"), "touch(7)");
    system("dd if=/dev/urandom of=" + cwd + "/._from/1/2/3/4KiB bs=1k count=4 >& /dev/null");
    system("dd if=/dev/urandom of=" + cwd + "/._from/4/5/6/1MiB bs=1k count=1k >& /dev/null");
    system("dd if=/dev/urandom of=" + cwd + "/._from/sparse-1MiB bs=1k seek=1023 count=1 >& /dev/null");

    TEST_NO_EXP(fs->deep_copy(cwd + "/._from", cwd + "/._to"), "deep_copy(1)");
    system("cd " + cwd + "/._from && /bin/ls -lnR --time-style=+%s | egrep -v 'xxx|total' > xxx");
    system("cd " + cwd + "/._to && /bin/ls -lnR --time-style=+%s | egrep -v 'xxx|total' > xxx");
    TEST_NO_EXP(s1 = fs->file_get_contents(cwd + "/._from/xxx"), "file_get_contents(1)");
    TEST_NO_EXP(s2 = fs->file_get_contents(cwd + "/._to/xxx"), "file_get_contents(2)");
    TEST(s1, s2, "deep_copy(2)");

    system("rm -f ._from/xxx ._to/xxx");
    system("cd ._from && find . -type f | xargs -n 1 -I FILE cmp FILE ../._to/FILE >& " + cwd + "/._test");
    TEST_NO_EXP(stmp = fs->file_get_contents(cwd + "/._test"), "file_get_contents(3)");
    TEST(stmp.Trim(), "", "deep_copy(3)");
    system("rm -rf ._from ._to ._test");

    // done
    FTrace::disable();
    cout << "All pass:" << (all_pass ? ANSI_PASS : ANSI_FAIL) << endl;
    return (all_pass ? 0 : 1);
}
