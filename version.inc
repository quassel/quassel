//! This is the fallback version number in case we can't autogenerate one
baseVersion = "0.5-pre";

protocolVersion = 9;       //< Version of the client/core protocol
coreNeedsProtocol   = 9;   //< Minimum protocol version the core needs
clientNeedsProtocol = 9;   //< Minimum protocol version the client needs

distCommittish = $Format:%H$
distCommitDate = $Format:%at$
