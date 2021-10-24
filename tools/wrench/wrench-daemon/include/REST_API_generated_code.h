	request_handlers["advanceTime"] = [sc](json data) { return sc->advanceTime(std::move(data)); };
	request_handlers["getTime"] = [sc](json data) { return sc->getSimulationTime(std::move(data)); };
	request_handlers["waitForNextSimulationEvent"] = [sc](json data) { return sc->waitForNextSimulationEvent(std::move(data)); };
	request_handlers["getSimulationEvents"] = [sc](json data) { return sc->getSimulationEvents(std::move(data)); };
	request_handlers["getAllHostnames"] = [sc](json data) { return sc->getAllHostnames(std::move(data)); };
	request_handlers["standardJobGetTasks"] = [sc](json data) { return sc->getCompoundJobTasks(std::move(data)); };
	request_handlers["addBareMetalComputeService"] = [sc](json data) { return sc->addBareMetalComputeService(std::move(data)); };
	request_handlers["addSimpleStorageService"] = [sc](json data) { return sc->addSimpleStorageService(std::move(data)); };
	request_handlers["createFileCopyAtStorageService"] = [sc](json data) { return sc->createFileCopyAtStorageService(std::move(data)); };
	request_handlers["addFileRegistryService"] = [sc](json data) { return sc->addFileRegistryService(std::move(data)); };
	request_handlers["createCompoundJob"] = [sc](json data) { return sc->createCompoundJob(std::move(data)); };
	request_handlers["submitCompoundJob"] = [sc](json data) { return sc->submitCompoundJob(std::move(data)); };
	request_handlers["createTask"] = [sc](json data) { return sc->createTask(std::move(data)); };
	request_handlers["taskGetFlops"] = [sc](json data) { return sc->getTaskFlops(std::move(data)); };
	request_handlers["taskGetMinNumCores"] = [sc](json data) { return sc->getTaskMinNumCores(std::move(data)); };
	request_handlers["taskGetMaxNumCores"] = [sc](json data) { return sc->getTaskMaxNumCores(std::move(data)); };
	request_handlers["taskGetMemory"] = [sc](json data) { return sc->getTaskMemory(std::move(data)); };
	request_handlers["addFile"] = [sc](json data) { return sc->addFile(std::move(data)); };
	request_handlers["fileGetSize"] = [sc](json data) { return sc->getFileSize(std::move(data)); };
	request_handlers["addInputFile"] = [sc](json data) { return sc->addInputFile(std::move(data)); };
	request_handlers["addOutputFile"] = [sc](json data) { return sc->addOutputFile(std::move(data)); };
	request_handlers["getTaskInputFiles"] = [sc](json data) { return sc->getTaskInputFiles(std::move(data)); };
	request_handlers["getInputFiles"] = [sc](json data) { return sc->getInputFiles(std::move(data)); };
	request_handlers["stageInputFiles"] = [sc](json data) { return sc->stageInputFiles(std::move(data)); };
