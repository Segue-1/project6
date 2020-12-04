#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#define MSGSZ 50

struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
};

int message_q_getter();
void message_q_remover(int msgqid);
void message_receiver(int msgqid, struct msgbuf* mbuf, int mtype);
void message_sender(int msgqid, struct msgbuf* mbuf, int mtype);

#endif
