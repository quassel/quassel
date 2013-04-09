//! This is the fallback version number in case we can't autogenerate one
baseVersion = "0.10-pre";

protocolVersion = 10;       //< Version of the client/core protocol
coreNeedsProtocol   = 10;   //< Minimum protocol version the core needs
clientNeedsProtocol = 10;   //< Minimum protocol version the client needs

distCommittish = $Format:%H$
distCommitDate = $Format:%at$
