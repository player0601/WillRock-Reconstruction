// test_msg.cpp -- (test) --msgtest: the rebuilt message system (src/engine/msgaddr.cpp, llist.cpp) on the real
// msgSystem. Four test addresses (tstADDR, test-only) are inserted with priorities and class ids, then: SendAll order,
// PostMsg + NextFrame + DispatchAll order, a delayed message, the lock count at +0x24, DestroyAddr with a destroy
// notify and the purge at the end of SendAll(purgeMsgId), RemoveAddr. Prints each step against what it should be.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../include/wr_sdk.h"
#include "test_assets.h"

static char tstLog[256];

class tstADDR : public msgADDR {
public:
    int  lock;                      // +0x24 (where the game's classes keep their lock count)
    char tag;
    tstADDR(char t, u32 cls, u32 prio)
    {
        node.prev = node.next = 0;
        flags = 0;
        magic = 0x4D414452;
        classId = cls;
        unk18 = 0;
        priority = prio;
        sign = 0;
        lock = 0;
        tag = t;
    }
    ~tstADDR()
    {
        magic = 0;
        size_t n = strlen(tstLog);
        _snprintf(tstLog + n, sizeof(tstLog) - n - 1, "~%c ", tag);
    }
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret)
    {
        size_t n = strlen(tstLog);
        if (id == MSG_MSG_DESTROY_ADDR)
            _snprintf(tstLog + n, sizeof(tstLog) - n - 1, "%c:destroy(%c) ", tag, ((tstADDR *)data)->tag);
        else
            _snprintf(tstLog + n, sizeof(tstLog) - n - 1, "%c:%d ", tag, id);
        *ret = tag;
        return 1;
    }
};

static void tstOrder(const char *what, const char *want)
{
    char got[64] = "";
    for (msgADDR *a = msgSystem.GetFirstAddr(); a; a = a->GetNext()) {
        size_t n = strlen(got);
        got[n] = ((tstADDR *)a)->tag;
        got[n + 1] = 0;
    }
    printf("msgtest: %-34s %-8s (want %s) %s, %d in list\n", what, got, want, strcmp(got, want) ? "BAD" : "ok",
           msgSystem.addrList.GetNumItems());
}

static void tstCheck(const char *what, const char *want)
{
    printf("msgtest: %-34s [%s] (want [%s]) %s\n", what, tstLog, want, strcmp(tstLog, want) ? "BAD" : "ok");
    tstLog[0] = 0;
}

int tstMsgTest()
{
    if (!msgSystem.Init()) {
        printf("msgtest: Init failed\n");
        return 1;
    }
    printf("msgtest: Init: capacity %d, %d names (%s), %d packers\n", msgSystem.capacity, msgSystem.nRegMsg,
           msgSystem.regMsg[2].name, msgSystem.nDataPack);
    msgSystem.purgeMsgId = 1000;
    tstADDR *a = new tstADDR('A', 0x41414141, 50);
    tstADDR *b = new tstADDR('B', 0x42424242, 10);
    tstADDR *c = new tstADDR('C', 0x41414141, 30);
    tstADDR *d = new tstADDR('D', 0x43434343, 30);
    msgSystem.InsertAddr(a, 0, 0);
    msgSystem.InsertAddr(b, 0, 0);
    msgSystem.InsertAddr(c, 0, 0);
    msgSystem.InsertAddr(d, 0, 0);
    tstOrder("InsertAddr by priority", "BCDA");
    printf("msgtest: class numbers A %u C %u (want 0 1), %d class ids (want 3)\n", a->unk18, c->unk18,
           msgSystem.addrList.nClass);

    msgSystem.SendAll(1000, 0, 0);
    tstCheck("SendAll(1000)", "B:1000 C:1000 D:1000 A:1000 ");
    printf("msgtest: SendMsg to C returns '%c'\n", msgSystem.SendMsg(c, 1005, 0, 0));
    tstLog[0] = 0;

    msgSystem.PostMsg(a, 1001, 0);
    msgSystem.PostMsg(b, 1001, 0);
    msgSystem.DispatchAll(0.0f);
    tstCheck("posted, before NextFrame", "");
    msgSystem.NextFrame();
    msgSystem.DispatchAll(0.0f);
    tstCheck("NextFrame + DispatchAll", "B:1001 A:1001 ");

    msgSystem.PostTimeMsg(c, 1002, 0, 0.5f);
    msgSystem.DispatchAll(0.3f);
    tstCheck("delayed 0.5 s, after 0.3 s", "");
    msgSystem.DispatchAll(0.3f);
    tstCheck("delayed 0.5 s, after 0.6 s", "C:1002 ");

    c->lock = 1;
    msgSystem.PostTimeMsg(c, 1003, 0, 0.1f);
    msgSystem.SendAll(1000, 0, 0);
    msgSystem.DispatchAll(0.5f);
    tstCheck("C locked: SendAll + delayed", "B:1000 D:1000 A:1000 ");
    c->lock = 0;
    msgSystem.DispatchAll(0.5f);
    tstCheck("C unlocked: delayed", "C:1003 ");

    a->flags |= MSGADDR_WANT_DESTROY_NOTIFY;
    msgSystem.PostMsg(d, 1004, 0);
    msgSystem.DestroyAddr(d);
    tstCheck("DestroyAddr(D)", "A:destroy(D) ");
    printf("msgtest: D flags %X (want C), pending %d (want 1)\n", d->flags, msgSystem.nPendingDestroy);
    msgSystem.SendAll(1000, 0, 0);
    tstCheck("SendAll(purge id)", "B:1000 C:1000 A:1000 ~D ");
    tstOrder("after the purge", "BCA");
    msgSystem.NextFrame();
    msgSystem.DispatchAll(0.0f);
    tstCheck("D's posted message", "");

    msgSystem.PostTimeMsg(b, 1006, 0, 1.0f);
    msgSystem.RemoveAddr(b);
    tstOrder("RemoveAddr(B)", "CA");
    printf("msgtest: time queue after RemoveAddr(B): %d (want 0)\n", msgSystem.timeQueue.GetNumItems());
    delete b;
    tstLog[0] = 0;

    // the shovel's swing message through its packer (plw_svl.cpp pwpSVL_PACK): a wall hit there and back
    {
        extern msgDATA_PACK pwpSvlPack;
#pragma pack(push, 1)
        struct { msgDATA h; u8 _04, hit; int target; u16 face, obj; m3dV point, dir; } m, *r;
#pragma pack(pop)
        msgSystem.RegisterMsgData(&pwpSvlPack);
        memset(&m, 0, sizeof(m));
        m.h.packId = (u8)pwpSvlPack.id;
        m.h.tag = 0x40;
        m.h.size = 0x26;
        m.hit = 1;
        m.target = 7;
        m.face = 33;
        m.obj = 5;
        m.point.x = 12.5f; m.point.y = 3.25f; m.point.z = -40.0f;
        m.dir.x = 0.0f; m.dir.y = 1.0f; m.dir.z = 0.0f;
        static u8 buf[256];
        fioFILE_MEM w;
        w.Init(buf, sizeof(buf), 0x12);
        int pk = msgSystem.Pack(&w, (msgDATA *)&m);
        fioFILE_MEM rd;
        rd.Init(buf, sizeof(buf), 0x11);
        r = (decltype(r))msgSystem.UnPack(&rd);
        printf("msgtest: shovel packer id %d: pack %d, back: target %d hit %d face %d obj %d point %.2f %.2f %.2f dir %.2f "
               "%.2f %.2f %s\n", pwpSvlPack.id, pk, r ? r->target : -9, r ? r->hit : -9, r ? r->face : -9, r ? r->obj : -9,
               r ? r->point.x : 0, r ? r->point.y : 0, r ? r->point.z : 0, r ? r->dir.x : 0, r ? r->dir.y : 0,
               r ? r->dir.z : 0,
               r && r->target == 7 && r->hit == 1 && r->face == 33 && r->obj == 5 && fabsf(r->point.x - 12.5f) < 0.05f &&
               fabsf(r->point.z + 40.0f) < 0.05f && fabsf(r->dir.y - 1.0f) < 0.05f ? "ok" : "BAD");
    }
    msgSystem.Term();
    printf("msgtest: done\n");
    return 0;
}
