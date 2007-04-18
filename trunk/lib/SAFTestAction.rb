module SAFTest

require 'SAFTestUtils'
require 'singleton'

class Actions < SAFTestUtils

    include Singleton
    include Enumerable

    def initialize()
        super()
        @dir = actionDir()
        @actions = []
        load()
    end

    def each()
        @actions.each {|action| yield(action)}
    end

    def length()
        return @actions.length
    end

    def addAction(name)
        id = 1
        while true do
            break unless @actions.find {|a| a.id == id.to_s}
            id += 1
        end
        id = id.to_s

        dir = File.join(@dir, id)
        Dir.mkdir(dir)
        action = Action.new(id, dir, name)
        action.store()
        @actions << action

        return action
    end

    def deleteAction(id)
        action = @actions.find {|a| a.id == id}
        @actions.delete(action)
        system("rm -rf " + File.join(@dir, id))
    end

    def deleteAllActions()
        ids = @actions.map {|action| action.id}
        ids.each do |id|
            deleteAction(id)
        end
    end

    def getAction(id)
        return @actions.find {|a| a.id == id}
    end

    def load()
        @actions = []
        actionDirs = Dir.glob(File.join(@dir, "*"))
        actionDirs.sort! do |d1, d2|
            File.basename(d1).to_i <=> File.basename(d2).to_i
        end
        actionDirs.each do |actionDir|
            id = File.basename(actionDir)
            action = Action.new(id, actionDir)
            action.load()
            @actions << action
        end
    end

end # class Actions

class Action < SAFTestUtils

    def initialize(id, dir, name=nil)
        super()
        @id, @dir, @name = id, dir, name
        @commands = []
        @state = "pending"
        @result = ""
        @startTime = @stopTime = nil
    end

    attr_reader :id, :name, :commands, :state, :result, :startTime, 
        :stopTime

    def addCommand(command, name=nil)
        id = @commands.size.to_s
        @commands << Command.new(id, File.join(@dir, id), command, name)
        store()
    end

    def getCommand(id)
        return @commands.find {|c| c.id.to_s == id.to_s}
    end

    def start()
        store()
        cmd = File.join(ENV['SAFTEST_ROOT'] + "/bin", "saftest_action") + " #{@id}"
        fork do
            Process.setsid()
            # need to close all file descriptors
            fork do
                logFile = File.join(@dir, "log")
                f = File.open(logFile, 'w')
                $stdin.close()
                $stdout.reopen(f)
                $stderr.reopen(f)
                exec(cmd)
            end
            exit 0
        end
    end

    def run()
        @state = 'running'
        @startTime = Time.now
        store()
        keepRunning = true
        @commands.each do |command|
            if keepRunning then
                command.run()
                if command.result != "0" then
                    keepRunning = false
                end
            else
                command.skip()
            end
        end
        @state = 'done'
        if keepRunning then
            @result = "succeeded"
        else
            @result = "failed"
        end
        @stopTime = Time.now
        store()
    end

    def store()
        Dir.mkdir(@dir) unless FileTest.directory?(@dir)
        path = File.join(@dir, "state")
        f = File.open(path, 'w')
        f.print("name=#{@name}\n")
        f.print("state=#{@state}\n")
        f.print("result=#{@result}\n")
        f.print("startTime=#{@startTime.tv_sec}\n") if @startTime
        f.print("stopTime=#{@stopTime.tv_sec}\n") if @stopTime
        f.close

        @commands.each do |command|
            command.store()
        end
    end

    def load()
        path = File.join(@dir, "state")
        return unless File.exists?(path)
        f = File.open(path, 'r')
        f.each_line do |line|
            line.chomp!()
            name, value = line.split('=', 2)
            case name
            when 'name'; @name = value
            when 'state'; @state = value
            when 'result'; @result = value
            when 'startTime'; @startTime = Time.at(value.to_i)
            when 'stopTime'; @stopTime = Time.at(value.to_i)
            end
        end

        @commands = []
        commandDirs = Dir.glob(File.join(@dir, "*")).sort
        commandDirs.each do |commandDir|
            next unless FileTest.directory?(commandDir)
            id = File.basename(commandDir)
            command = Command.new(id, commandDir)
            command.load()
            @result = command.result unless @result != "0"
            @commands << command
        end
    end

end # class Action

class Command

    def initialize(id, dir, command=nil, name=nil)
        @id, @dir, @command, @name = id, dir, command, name
        @state = "pending"
        @startTime = @stopTime = nil
        @result = ""
    end

    attr_reader :id, :command, :name, :state, :result, :startTime, :stopTime

    def run()
        @state = "running"
        @startTime = Time.now
        store()

        print "# #{@command}\n"
        readme, writeme = IO.pipe
        pid = fork {
            # child
            $stdout.reopen(writeme)
            $stderr.reopen(writeme)
            readme.close
            exec(@command)
        }
        # parent
        writeme.close

        print "pid is #{pid}\n"

        f = File.open(File.join(@dir, "output"), 'w')
        while readme.gets do
            f.write($_)
            f.flush
            begin
                mypid, status = Process.waitpid2(pid, 
                                      Process::WNOHANG | Process::WUNTRACED)
            rescue
            end
        end
    
        begin
            mypid, status = Process.waitpid2(pid, Process::WUNTRACED)
            rescue
        end

        @result = status.exitstatus
        f.close

        @state = "done"
        @stopTime = Time.now
        store()
    end

    def skip()
        @state = "skipped"
    end

    def output()
        output = nil
        begin
            f = File.open(File.join(@dir, "output"))
            output = f.read()
            f.close()
        rescue
            output = nil
        end
        return output
    end

    def store()
        Dir.mkdir(@dir) unless FileTest.directory?(@dir)
        path = File.join(@dir, "state")
        f = File.open(path, 'w')
        f.print("name=#{@name}\n")
        f.print("command=#{@command}\n")
        f.print("state=#{@state}\n")
        f.print("result=#{@result}\n")
        f.print("startTime=#{@startTime.tv_sec}\n") if @startTime
        f.print("stopTime=#{@stopTime.tv_sec}\n") if @stopTime
        f.close
    end

    def load()
        path = File.join(@dir, "state")
        f = File.open(path, 'r')
        f.each_line do |line|
            line.chomp!
            name, value = line.split('=', 2)
            case name
            when 'name'; @name = value
            when 'command'; @command = value
            when 'state'; @state = value
            when 'result'; @result = value
            when 'startTime'; @startTime = Time.at(value.to_i)
            when 'stopTime'; @stopTime = Time.at(value.to_i)
            end
        end
        f.close
    end

end # class Command

end # module SAFTest
