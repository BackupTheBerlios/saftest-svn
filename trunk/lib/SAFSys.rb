module SAFTest

# The SAFSys class should not be dependent on any other SAFTest classes
class SAFSys
    require 'socket'

    def initialize()
    end

    def generateCommand(cmd, node=nil, user=nil)
        return cmd + ' 2>&1' if node == nil
        # Find out what user/password combination to use for this node.
        # This is stored in the configuration file.
        user = 'root'
        password = ''
        remoteCmd = "export SAF_ROOT=%s; %s 2>&1; echo \"+EXITCODE+:$?\"" %
            [ENV['SAF_ROOT'], cmd]
        if node != nil
            result = "ssh %s@%s '%s'" % [user, node, remoteCmd]
            return result
        end
    end

    def runCommand(cmd, node=nil, user=nil)
        cmd = '|' + generateCommand(cmd, node, user)
        exitCode = 0
        file = open(cmd, 'r')
        begin
            while ! file.eof? do
                line = file.readline
                if node != nil and line =~ /^\+EXITCODE\+:(\d+)$/ then
                    exitCode = $1.to_i
                    break
                else
                    if iterator? then
                        yield line
                    else
                        print line
                    end
                end
            end
        ensure
            file.close
        end
        exitCode = $? >> 8 if exitCode == 0
        return exitCode
    end

    def captureCommand(cmd, node=nil, user=nil)
        output = []
        exitCode = runCommand(cmd, node, user) do |line|
            output << line
        end
        return [exitCode, output]
    end

    def fullHostname()
        return Socket.gethostname()
    end

    def simpleHostname()
        name = Socket.gethostname()
        if ((name =~ /([A-z0-9]+)\..*/) != nil )
            return $1
        else
            return name
        end
    end

end # class SAFSys

end # module SAFTest
