#include "Forte.h"
#include "ansi_test.h"

// main
int main(int argc, char *argv[])
{
    FileSystem fs;
    FString stmp, cwd, s1, s2;
    struct stat st;
    int err = 0;

    // logging
    LogManager log_mgr;
    log_mgr.BeginLogging("filesystem.log");
    log_mgr.SetGlobalLogMask(HLOG_NODEBUG & ~(HLOG_INFO));

    // init trace
    FTrace::Enable();

    // test path functions
    TEST(fs.MakeRelativePath("/foo/bar/baz", "/foo/fred/barney"),
         "../../fred/barney",
         "MakeRelativePath(1)");

    TEST(fs.MakeRelativePath("/foo/bar/baz", "/fred/barney"),
         "../../../fred/barney",
         "MakeRelativePath(2)");

    TEST(fs.MakeRelativePath("/foo/bar/baz", "/foo/bar/baz/fred"),
         "fred",
         "MakeRelativePath(3)");

    TEST(fs.MakeRelativePath("/foo/bar/baz", "/foo/bar/baz/fred/barney"),
         "fred/barney",
         "MakeRelativePath(4)");

    TEST(fs.MakeRelativePath("/foo/bar/baz", "/foo/bar/baz/./fred/barney"),
         "./fred/barney",
         "MakeRelativePath(5)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "../../fred/barney"),
         "/foo/fred/barney",
         "ResolveRelativePath(1)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "../../../fred/barney"),
         "/fred/barney",
         "ResolveRelativePath(2)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "fred"),
         "/foo/bar/baz/fred",
         "ResolveRelativePath(3)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "fred/barney"),
         "/foo/bar/baz/fred/barney",
         "ResolveRelativePath(4)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "./fred/barney"),
         "/foo/bar/baz/./fred/barney",
         "ResolveRelativePath(5)");

    TEST(fs.ResolveRelativePath("/foo/bar/baz", "/fred/barney"),
         "/fred/barney",
         "ResolveRelativePath(6)");

    // test GetCWD
    TEST_NO_EXP(cwd = fs.GetCWD(), "GetCWD(1)");
    TEST(cwd, getenv("PWD"), "GetCWD(2)");

    // test SymLink functions
    system("Touch ._baz");
    TEST_NO_EXP(fs.SymLink("._baz", "._bar"), "SymLink(1)");
    TEST_NO_EXP(fs.SymLink("._bar", "._foo"), "SymLink(2)");
    TEST_NO_EXP(fs.SymLink("/nowhere", "._fred"), "SymLink(3)");

    TEST(fs.ResolveSymLink("._bar"), cwd + "/._baz", "ResolveSymLink(1)");
    TEST(fs.ResolveSymLink("._foo"), cwd + "/._baz", "ResolveSymLink(2)");
    TEST_NO_EXP(fs.Unlink("._baz"), "Unlink(1)");
    TEST_EXP(fs.ResolveSymLink("._foo"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||" + cwd + "/._baz",
             "ResolveSymLink(3)");
    TEST_NO_EXP(fs.Unlink("._bar"), "Unlink(2)");
    TEST_EXP(fs.ResolveSymLink("._foo"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||" + cwd  + "/._bar",
             "ResolveSymLink(4)");
    TEST_NO_EXP(fs.Unlink("._foo"), "Unlink(3)");
    TEST_EXP(fs.ResolveSymLink("._foo"), "FORTE_RESOLVE_SYMLINK_BROKEN|||._foo|||._foo",
             "ResolveSymLink(5)");
    TEST_NO_EXP(fs.Unlink("._fred"), "Unlink(4)");
    TEST_NO_EXP(fs.Unlink("._fred"), "Unlink(5)");

    symlink("._loop0", "._loop1");
    symlink("._loop1", "._loop0");
    TEST_EXP(fs.ResolveSymLink("._loop0"),
             "FORTE_RESOLVE_SYMLINK_LOOP|||._loop0|||" + cwd + "/._loop1",
             "ResolveSymLink(6)");
    TEST_EXP(fs.ResolveSymLink("._loop1"),
             "FORTE_RESOLVE_SYMLINK_LOOP|||._loop1|||" + cwd + "/._loop0",
             "ResolveSymLink(7)");
    TEST_NO_EXP(fs.Unlink("._loop0"), "Unlink(6)");
    TEST_NO_EXP(fs.Unlink("._loop1"), "Unlink(7)");

    // test Link to broken SymLink
    TEST_NO_EXP(fs.SymLink("broken", "broken.SymLink"), "SymLink(4)");
    TEST(fs.ReadLink("broken.SymLink"), "broken", "ReadLink(1)");
    TEST_EXP(fs.ReadLink("not.a.SymLink"),
             "FORTE_READ_SYMLINK_FAIL|||not.a.SymLink|||No such file or directory",
             "ReadLink(2)");
    TEST_NO_EXP(fs.Link("broken.SymLink", "broken.SymLink.hardLink"), "Link(1)");
    TEST_NO_EXP(fs.Unlink("broken.SymLink"), "Unlink(8)");
    TEST_NO_EXP(fs.Unlink("broken.SymLink.hardLink"), "Unlink(9)");

    // test double-resolve
    TEST_NO_EXP(fs.MakeDir("._dir0"), "MakeDir(1)");
    TEST_NO_EXP(fs.SymLink("._dir0/back", "._forward"), "SymLink(5)");
    TEST_NO_EXP(fs.SymLink("../._foo", "._dir0/back"), "SymLink(6)");
    system("Touch ._foo");
    TEST(fs.ResolveSymLink("._forward"), cwd + "/._foo", "ResolveSymLink(8)");
    TEST_NO_EXP(fs.Unlink("._foo"), "Unlink(10)");
    TEST_NO_EXP(fs.Unlink("._forward"), "Unlink(11)");
    TEST_EXP(fs.Unlink("._dir0", false), "FORTE_UNLINK_FAIL|||._dir0|||Directory not empty",
             "Unlink(13)");
    TEST_NO_EXP(fs.Unlink("._dir0", true), "Unlink(12)");
    TEST_NO_EXP(err = fs.Stat("._dir0", &st), "stat(1)");
    TEST(err, -1, "Unlink(14)");
    TEST_NO_EXP(err = fs.Stat(".", &st), "stat(2)");
    TEST(err, 0, "stat(3)");
    TEST(S_ISDIR(st.st_mode), true, "stat(4)");

    // test full resolve
    TEST_NO_EXP(fs.MakeDir("._dir0"), "MakeDir(2)");
    TEST_NO_EXP(fs.MakeDir("._dir1"), "MakeDir(3)");
    TEST_NO_EXP(fs.MakeDir("._dir1"), "MakeDir(4)");
    TEST_NO_EXP(fs.Touch("._dir0/foo"), "Touch(1)");
    TEST_NO_EXP(fs.SymLink("._dir0", "._s0"), "SymLink(7)");
    TEST_NO_EXP(fs.SymLink("._dir1", "._s1"), "SymLink(8)");
    TEST_NO_EXP(fs.SymLink("._s0/foo", "._s2"), "SymLink(9)");
    TEST_NO_EXP(fs.SymLink("../._s2", "._dir1/._s3"), "SymLink(10)");
    TEST_NO_EXP(fs.SymLink("._s1/._s3", "._s4"), "SymLink(11)");
    TEST_NO_EXP(stmp = fs.FullyResolveSymLink("._s2"), "FullyResolveSymLink(1)");
    TEST(stmp, cwd + "/._dir0/foo", "FullyResolveSymLink(2)");
    TEST_NO_EXP(stmp = fs.FullyResolveSymLink("._dir1/._s3"), "FullyResolveSymLink(3)");
    TEST(stmp, cwd + "/._dir0/foo", "FullyResolveSymLink(4)");
    TEST_NO_EXP(stmp = fs.FullyResolveSymLink("._s4"), "FullyResolveSymLink(5)");
    TEST(stmp, cwd + "/._dir0/foo", "FullyResolveSymLink(6)");
    system("rm -f ._dir0/foo");
    TEST_EXP(fs.FullyResolveSymLink("._s4"),
             "FORTE_RESOLVE_SYMLINK_BROKEN|||._s4|||" + cwd + "/._dir0/foo",
             "FullyResolveSymLink(7)");
    system("rm -rf ._dir0 ._dir1 ._s0 ._s1 ._s2 ._s4");

    // test Touch
    TEST_NO_EXP(fs.Touch(cwd + "/._foo"), "Touch(2)");
    TEST_NO_EXP(fs.Touch(cwd + "/._foo"), "Touch(3)");
    TEST_NO_EXP(fs.Touch("._foo"), "Touch(4)");
    system("rm -f ._foo");

    // test DeepCopy
    TEST_NO_EXP(fs.MakeDir(cwd + "/._from/1/2/3", 0777, true), "MakeDir(5)");
    TEST_NO_EXP(fs.MakeDir(cwd + "/._from/4/5/6", 0777, true), "MakeDir(6)");
    TEST_NO_EXP(fs.Touch(cwd + "/._from/empty"), "Touch(5)");
    TEST_NO_EXP(fs.Touch(cwd + "/._from/1/empty"), "Touch(6)");
    TEST_NO_EXP(fs.Touch(cwd + "/._from/4/5/empty"), "Touch(7)");
    system("dd if=/dev/urandom of=" + cwd + "/._from/1/2/3/4KiB bs=1k count=4 >& /dev/null");
    system("dd if=/dev/urandom of=" + cwd + "/._from/4/5/6/1MiB bs=1k count=1k >& /dev/null");
    system("dd if=/dev/urandom of=" + cwd + "/._from/sparse-1MiB bs=1k seek=1023 count=1 >& /dev/null");

    TEST_NO_EXP(fs.DeepCopy(cwd + "/._from", cwd + "/._to"), "DeepCopy(1)");
    system("cd " + cwd + "/._from && /bin/ls -lnR --time-style=+%s | egrep -v 'xxx|total' > xxx");
    system("cd " + cwd + "/._to && /bin/ls -lnR --time-style=+%s | egrep -v 'xxx|total' > xxx");
    TEST_NO_EXP(s1 = fs.FileGetContents(cwd + "/._from/xxx"), "FileGetContents(1)");
    TEST_NO_EXP(s2 = fs.FileGetContents(cwd + "/._to/xxx"), "FileGetContents(2)");
    TEST(s1, s2, "DeepCopy(2)");

    system("rm -f ._from/xxx ._to/xxx");
    system("cd ._from && find . -type f | xargs -n 1 -I FILE cmp FILE ../._to/FILE >& " + cwd + "/._test");
    TEST_NO_EXP(stmp = fs.FileGetContents(cwd + "/._test"), "FileGetContents(3)");
    TEST(stmp.Trim(), "", "DeepCopy(3)");
    system("rm -rf ._from ._to ._test");

    // done
    FTrace::Disable();
    cout << "All pass:" << (all_pass ? ANSI_PASS : ANSI_FAIL) << endl;
    return (all_pass ? 0 : 1);
}
