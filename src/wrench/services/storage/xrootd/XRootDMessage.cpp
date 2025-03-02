/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "wrench/services/storage/xrootd/XRootDMessage.h"

#include <utility>

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


namespace wrench {
    namespace XRootD {
        /**
         * @brief Constructor
         * @param payload: the message size in bytes
         */
        Message::Message(sg_size_t payload) : StorageServiceMessage(payload) {}
        /**
         * @brief Constructor
         * @param answer_commport: The commport the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param file: The file to search for
         * @param node: The node where the search was initiated
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         * @param timeToLive: The max number of hops this message can take
         */
        ContinueSearchMessage::ContinueSearchMessage(S4U_CommPort *answer_commport,
                                                     std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                                     std::shared_ptr<DataFile> file,
                                                     Node *node,
                                                     sg_size_t payload,
                                                     std::shared_ptr<bool> answered,
                                                     int timeToLive) : Message(payload), answer_commport(answer_commport), original(std::move(std::move(original))), file(std::move(std::move(file))), node(node), answered(std::move(std::move(answered))), timeToLive(timeToLive) {}
        /**
         * @brief Constructor
         * @param answer_commport: The commport the final answer should be sent to
         * @param file: The file being searched for
         * @param fileReadRequest: Whether this message is in response to a file read request (true) or a file lookup request (false)
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem

         */
        FileNotFoundAlarm::FileNotFoundAlarm(S4U_CommPort *answer_commport,
                                             std::shared_ptr<DataFile> file,
                                             bool fileReadRequest,
                                             std::shared_ptr<bool> answered) : Message(0), answer_commport(answer_commport), file(std::move(std::move(file))), fileReadRequest(fileReadRequest), answered(std::move(std::move(answered))) {}
        /**
        * @brief Copy Constructor
        * @param other: The message to copy.  timeToLive is decremented
        */
        ContinueSearchMessage::ContinueSearchMessage(ContinueSearchMessage *other) : Message(other->payload), answer_commport(other->answer_commport), original(other->original), file(other->file), node(other->node), answered(other->answered), timeToLive(other->timeToLive - 1) {}

        /**
         * @brief Constructor
         * @param answer_commport: The commport the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param node: The node where the search was initiated
         * @param file: The file that was found
         * @param locations: All locations that where found in this subtree
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         */
        UpdateCacheMessage::UpdateCacheMessage(S4U_CommPort *answer_commport, std::shared_ptr<StorageServiceFileReadRequestMessage> original, Node *node, std::shared_ptr<DataFile> file, std::set<std::shared_ptr<FileLocation>> locations,
                                               sg_size_t payload, std::shared_ptr<bool> answered) : Message(payload), answer_commport(answer_commport), original(std::move(std::move(original))), file(std::move(std::move(file))), locations(std::move(std::move(locations))), node(node), answered(std::move(std::move(answered))) {}
        /**
        * @brief Pointer Copy Constructor
        * @param other: The message to copy.
        */
        UpdateCacheMessage::UpdateCacheMessage(UpdateCacheMessage *other) : UpdateCacheMessage(*other) {}
        /**
        * @brief Reference Copy Constructor
        * @param other: The message to copy.
        */
        UpdateCacheMessage::UpdateCacheMessage(UpdateCacheMessage &other) : Message(other.payload), answer_commport(other.answer_commport), original(other.original), file(other.file), locations(other.locations), node(other.node), answered(other.answered) {}
        /**
        * @brief Constructor
        * @param file: The file to delete.
        * @param payload: the message size in bytes
        * @param timeToLive:  The max number of hops this message can take
        */
        RippleDelete::RippleDelete(std::shared_ptr<DataFile> file, sg_size_t payload, int timeToLive) : Message(payload), file(std::move(std::move(file))), timeToLive(timeToLive){}
        /**
        * @brief Copy Constructor
        * @param other: The message to copy.
        */
        RippleDelete::RippleDelete(RippleDelete *other) : Message(other->payload), file(other->file), timeToLive(other->timeToLive - 1) {}

        /**
        * @brief External Copy Constructor
        * @param other: The storage service file delete message to copy.
        * @param timeToLive:  The max number of hops this message can take
        */
        RippleDelete::RippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive) : Message(other->payload), file(other->location->getFile()), timeToLive(timeToLive) {}

        /**
         * @brief Constructor
         * @param answer_commport: The commport the final answer should be sent to
         * @param original: The original file read request being responded too.  If this is a file locate search, this should be null
         * @param file: The file to search for
         * @param node: The node where the search was initiated
         * @param payload: The message size in bytes
         * @param answered: A shared boolean for if the answer has been sent to the client.  This should be the same for all messages searching for this request.  Used to prevent the multiple response problem
         * @param timeToLive: The max number of hops this message can take
         * @param search_stack:  The available paths to the file
         */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(S4U_CommPort *answer_commport, std::shared_ptr<StorageServiceFileReadRequestMessage> original,
                                                                     std::shared_ptr<DataFile> file, Node *node, sg_size_t payload, std::shared_ptr<bool> answered, int timeToLive, std::vector<std::stack<Node *>> search_stack) : ContinueSearchMessage(answer_commport, std::move(original), std::move(file), node, payload, std::move(answered), timeToLive), search_stack(std::move(std::move(search_stack))){};
        /**
        * @brief Pointer Copy Constructor with auxiliary stack
        * @param toCopy: The message to copy, timeToLive is decremented
        * @param search_stack:  The available paths to the file
        */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(ContinueSearchMessage *toCopy, std::vector<std::stack<Node *>> search_stack) : ContinueSearchMessage(toCopy), search_stack(std::move(std::move(search_stack))){};

        /**
        * @brief Pointer Copy Constructor
        * @param toCopy: The message to copy, timeToLive is decremented
        */
        AdvancedContinueSearchMessage::AdvancedContinueSearchMessage(AdvancedContinueSearchMessage *toCopy) : ContinueSearchMessage(toCopy), search_stack(toCopy->search_stack){};

        /**
        * @brief Constructor
        * @param file: The file to delete.
        * @param payload: the message size in bytes
        * @param timeToLive:  The max number of hops this message can take
        * @param search_stack:  The available paths to the file
        */
        AdvancedRippleDelete::AdvancedRippleDelete(std::shared_ptr<DataFile> file, sg_size_t payload, int timeToLive, std::vector<std::stack<Node *>> search_stack) : RippleDelete(std::move(file), payload, timeToLive), search_stack(std::move(std::move(search_stack))) {}

        /**
        * @brief Copy Constructor with auxiliary stack
        * @param other: The message to copy.
        * @param search_stack:  The available paths to the file
        */
        AdvancedRippleDelete::AdvancedRippleDelete(RippleDelete *other, std::vector<std::stack<Node *>> search_stack) : RippleDelete(other), search_stack(std::move(std::move(search_stack))){};

        /**
        * @brief Copy Constructor
        * @param other: The message to copy.
        */
        AdvancedRippleDelete::AdvancedRippleDelete(AdvancedRippleDelete *other) : RippleDelete(other), search_stack(other->search_stack){};

        /**
         * @brief External Copy Constructor
         * @param other: The storage service file delete message to copy.
         * @param timeToLive:  The max number of hops this message can take
         * @param search_stack:  The available paths to the file
         */
        AdvancedRippleDelete::AdvancedRippleDelete(StorageServiceFileDeleteRequestMessage *other, int timeToLive, std::vector<std::stack<Node *>> search_stack) : RippleDelete(other, timeToLive), search_stack(std::move(std::move(search_stack))){};
    }// namespace XRootD
};   // namespace wrench
