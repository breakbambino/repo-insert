

#ifndef REPO_COMMAND_TLV_H
#define REPO_COMMAND_TLV_H

enum RepoTypeNumber {
    START_BLOCK_ID = 204,
    END_BLOCK_ID = 205,
    PROCESS_ID = 206,
    REPO_STATUS_CODE = 208,
    INSERT_NUM = 209,
    DELETE_NUM = 210,
};

#define BEST_ROUTE "/localhost/nfd/strategy/best-route"
#define MULTICAST "/localhost/nfd/strategy/multicast"

#endif // REPO_COMMAND_TLV_H