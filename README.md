# repo-insert
NDN Repo Integration into NDN-Lite. This example enable you to insert Data into a local Repo

## Steps
* Install [NDN-Repo](https://github.com/JonnyKong/ndn-python-repo)
* Modify Repo configurations here based on your situation. If you all use default settings, ``home_prefix``, ``repo_name`` should be OK.
```
    class Options
    {
        public:
        Options()
            : home_prefix("/ndn-iot")
            , repo_name("/testrepo")
            , service(NDN_SD_LED)
            , identifier("/livingroom")
        {
        }
        public:
        ndn::Name home_prefix;
        ndn::Name repo_name;
        uint8_t service;
        ndn::Name identifier;
    };
```
* Setting up route for home prefix (assuming only one publisher) and Repo. You should first get their face ids on NFD. 
```
    m_client->registerRoute(m_options.home_prefix, 404);
    m_client->registerRoute("/testrepo", 368);
```
* ``make`` and ``./repo``. It may need keychain access.
